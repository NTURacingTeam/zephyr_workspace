#include "nturt/msg.h"

// glibc includes
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// zephyr includes
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

// lib includes
#include <canopennode.h>

// project includes
#include "nturt/err.h"

LOG_MODULE_REGISTER(nturt_msg, CONFIG_NTURT_LOG_LEVEL);

/* macro ---------------------------------------------------------------------*/
/// @brief Size of work buffer for EMCY callback.
#define EMCY_WORK_BUF_SIZE 5

/* type ----------------------------------------------------------------------*/
/// @brief Structure for global variables of message module.
struct msg {
  struct canopen *co;

#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE
  node_mon_t *mon;
#endif  // CO_CONFIG_HB_CONS
};

/// @brief Arguments for bottom half of @ref emcy_cb.
struct emcy_cb_args {
  uint16_t ident;
  uint16_t errorCode;
  uint8_t errorRegister;
  uint8_t errorBit;
  uint32_t infoCode;
};

/* static function declaration -----------------------------------------------*/
/// @brief Callback function when receiving CANopen EMCY object.
static void emcy_cb(uint16_t ident, uint16_t errorCode, uint8_t errorRegister,
                    uint8_t errorBit, uint32_t infoCode);

/// @brief Bottom half of @ref emcy_cb.
static void emcy_work(struct k_work *work);

/// @brief Callback function when receiving from error channel.
static void err_chan_cb(const struct zbus_channel *chan);

#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE
/// @brief Callback function when monitored node NMT state changed.
static void hb_mnt_changed_cb(uint8_t id, uint8_t idx,
                              CO_NMT_internalState_t state, void *arg);

/// @brief Callback function when monitored node heartbeat started for the first
/// time after timeout.
static void hb_started_cb(uint8_t id, uint8_t idx, void *arg);

/// @brief Callback function when monitored node heartbeat timed out for the
/// first time.
static void hb_timeout_cb(uint8_t id, uint8_t idx, void *arg);

/// @brief Callback function when monitored node reset.
static void hb_reset_cb(uint8_t id, uint8_t idx, void *arg);

/**
 * @brief Convert node ID to corresponding error code.
 *
 * @param id Node ID.
 * @return Corresponding error code.
 */
static enum err_code node_mon_id_to_err_code(enum co_node_id id);

/**
 * @brief Update error code based on monitored node status.
 *
 * @param id Node ID.
 * @param mon Node monitor object.
 */
static void node_mon_update_err_code(enum co_node_id id, node_mon_t mon);
#endif  // CO_CONFIG_HB_CONS

/// @brief Initialization function for message emergency module.
static int msg_emcy_init();

/// @brief Initialization function for RPDO object dictionaries.
static int rpdo_od_init();

/// @brief Initialization function for TPDO object dictionaries.
static int tpdo_od_init();

/* static variable -----------------------------------------------------------*/
/// @brief Global variable of message module.
static struct msg msg = {
#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE
    .mon = (node_mon_t[]){[0 ... OD_CNT_ARR_1016 - 1] = NODE_MON_INITIAL},
#endif  // CO_CONFIG_HB_CONS
};

WORK_CTX_BUF_DEFINE(emcy_ctx, EMCY_WORK_BUF_SIZE, emcy_work, &msg,
                    struct emcy_cb_args);

ZBUS_LISTENER_DEFINE(nturt_msg_err_chan_listener, err_chan_cb);
ZBUS_CHAN_ADD_OBS(err_chan, nturt_msg_err_chan_listener, 0);

/* function definition -------------------------------------------------------*/
int msg_init(struct canopen *co) {
  int ret;

  msg.co = co;

  // must be called before canopen_init
  ret = rpdo_od_init();
  if (ret < 0) {
    LOG_ERR("Failed to initialize RPDO object dictionary: %s", strerror(-ret));
    err_set_errors(ERR_CODE_CAN, true);
    return ret;
  }

  ret = tpdo_od_init();
  if (ret < 0) {
    LOG_ERR("Failed to initialize TPDO object dictionary: %s", strerror(-ret));
    err_set_errors(ERR_CODE_CAN, true);
    return ret;
  }

  ret = canopen_init(co);
  if (ret < 0) {
    LOG_ERR("Failed to initialize CANopen: %s", strerror(-ret));
    err_set_errors(ERR_CODE_CAN, true);
    return ret;
  }

  msg_emcy_init();

  return 0;
}

int sdo_write(struct canopen *co, int node_id, int index, int subindex,
              const void *data, size_t size, int timeout) {
  if (co->CO == NULL || !co->CO->CANmodule->CANnormal) {
    LOG_ERR("CANopen not initialized");
    return -ENODEV;
  }

  if (co->CO->SDOclient->state != CO_SDO_ST_IDLE) {
    LOG_ERR("SDO client not idle");
    return -EBUSY;
  }

  CO_SDO_return_t ret;
  CO_SDOclient_t *sdo = co->CO->SDOclient;

  ret = CO_SDOclient_setup(sdo, CO_CAN_ID_SDO_CLI + node_id,
                           CO_CAN_ID_SDO_SRV + node_id, node_id);
  if (ret != CO_SDO_RT_ok_communicationEnd) {
    LOG_ERR("Failed to setup SDO client: %d", ret);
    return -EIO;
  }

  ret =
      CO_SDOclientDownloadInitiate(sdo, index, subindex, size, timeout, false);
  if (ret != CO_SDO_RT_ok_communicationEnd) {
    LOG_ERR("Failed to initiate SDO download: %d", ret);
    return -EIO;
  }

  CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;
  bool partial = true;
  size_t written = 0;

  do {
    if (partial) {
      written += CO_SDOclientDownloadBufWrite(
          sdo, (const uint8_t *)data + written, size - written);

      if (written == size) {
        partial = false;
      }
    }

    ret = CO_SDOclientDownload(sdo, 1000, false, partial, &abort_code, NULL,
                               NULL);

    k_sleep(K_MSEC(1));
  } while (ret > 0);

  if (ret < 0) {
    LOG_ERR("Failed to download SDO: %d, abort code: 0x%X", ret, abort_code);
    return -EIO;
  }

  return 0;
}

/* static function definition ------------------------------------------------*/
static void emcy_cb(uint16_t ident, uint16_t errorCode, uint8_t errorRegister,
                    uint8_t errorBit, uint32_t infoCode) {
  struct work_ctx *ctx = work_ctx_alloc(emcy_ctx, EMCY_WORK_BUF_SIZE);
  if (ctx != NULL) {
    struct emcy_cb_args *args = ctx->args;
    args->ident = ident;
    args->errorCode = errorCode;
    args->errorRegister = errorRegister;
    args->errorBit = errorBit;
    args->infoCode = infoCode;

    k_work_submit(&ctx->work);
  } else {
    LOG_ERR(
        "EMCY process queue full, dropping EMCY object from id: %d, err: %d.",
        ident, errorCode);
  }
}

static void emcy_work(struct k_work *work) {
  struct msg *msg = WORK_CTX(work);
  struct emcy_cb_args *args = WORK_CTX_ARGS(work);
  bool set = args->errorCode != CO_EMC_NO_ERROR;

  // frame id == 0 when EMCY from this node
  if (args->ident == 0) {
    // filter out manufacturer and heartbeat consumer errors since they are
    // handled by err module and heartbeat consumer callbacks, respectively
    if (args->errorBit >= CO_EM_MANUFACTURER_START ||
        args->errorBit == CO_EM_HEARTBEAT_CONSUMER ||
        args->errorBit == CO_EM_HB_CONSUMER_REMOTE_RESET) {
      return;
    }

    if (set) {
      LOG_ERR(
          "Received EMCY object from this node: error set: 0x%X, reg: 0x%X, "
          "error bit: 0x%X, info: %d",
          args->errorCode, args->errorRegister, args->errorBit, args->infoCode);
    } else {
      LOG_INF(
          "Received EMCY object from this node: error cleared, reg: 0x%X, "
          "error bit: 0x%X, info: %d",
          args->errorRegister, args->errorBit, args->infoCode);
    }

    static const uint8_t err_mask[CO_EM_MANUFACTURER_START / 8] = {0};
    bool err_status_set =
        memcmp(msg->co->CO->em->errorStatusBits, err_mask, sizeof(err_mask));
    err_t errors = err_get_errors();

    // have to check both set and err_status_set since we could only access
    // error status bits now and it might have been set and cleared before we
    // could process it
    if (set && err_status_set && !(errors & ERR_CODE_CAN)) {
      err_set_errors(ERR_CODE_CAN, true);
    } else if (!set && !err_status_set && errors & ERR_CODE_CAN) {
      err_set_errors(ERR_CODE_CAN, false);
    }
    return;
  }

  int id = args->ident - CO_CAN_ID_EMERGENCY;

  if (set) {
    LOG_ERR(
        "Received EMCY object from node 0x%X: error set: 0x%X, reg: 0x%X, "
        "error bit: 0x%X, info: %d",
        id, args->errorCode, args->errorRegister, args->errorBit,
        args->infoCode);
  } else {
    LOG_INF(
        "Received EMCY object from node 0x%X: error cleared, reg: 0x%X, error "
        "bit: 0x%X, info: %d",
        id, args->errorRegister, args->errorBit, args->infoCode);
  }

#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE
  int idx = CO_HBconsumer_getIdxByNodeId(msg->co->CO->HBcons, id);
  if (idx != -1) {
    if (args->errorRegister != 0) {
      msg->mon[idx] |= NODE_MON_ERR;
    } else {
      msg->mon[idx] &= ~NODE_MON_ERR;
    }

    node_mon_update_err_code(id, msg->mon[idx]);
  }
#endif  // CO_CONFIG_HB_CONS
}

static void err_chan_cb(const struct zbus_channel *chan) {
  err_t errors = *(err_t *)zbus_chan_const_msg(chan);
  bool set = FLAG_SET_AND_CLEAR(errors, ERR_CODE_SET);

  enum err_code code;
  ERR_CODE_FOR_EACH(errors, code) {
    CO_error(msg.co->CO->em, set, ERR_CODE_TO_CO_ERR_STATUS(code),
             ERR_CODE_TO_CO_ERR_CODE(code), 0);
  }
}

#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE

static void hb_mnt_changed_cb(uint8_t id, uint8_t idx,
                              CO_NMT_internalState_t state, void *arg) {
  node_mon_t *mon = arg;

  if (state != CO_NMT_OPERATIONAL) {
    mon[idx] |= NODE_MON_NOT_OPT;
    LOG_WRN("Node %d NMT state changed to non-operational", id);

  } else {
    mon[idx] &= ~NODE_MON_NOT_OPT;
    LOG_INF("Node %d NMT state changed to operational", id);
  }

  node_mon_update_err_code(id, mon[idx]);
}

static void hb_started_cb(uint8_t id, uint8_t idx, void *arg) {
  node_mon_t *mon = arg;

  mon[idx] &= ~NODE_MON_TIMEOUT;
  LOG_INF("Node %d heartbeat active", id);

  node_mon_update_err_code(id, mon[idx]);
}

static void hb_timeout_cb(uint8_t id, uint8_t idx, void *arg) {
  node_mon_t *mon = arg;

  mon[idx] |= NODE_MON_TIMEOUT;
  LOG_ERR("Node %d heartbeat timeout", id);

  node_mon_update_err_code(id, mon[idx]);
}

static void hb_reset_cb(uint8_t id, uint8_t idx, void *arg) {
  node_mon_t *mon = arg;

  mon[idx] = NODE_MON_INITIAL;
  LOG_INF("Node %d reset", id);

  node_mon_update_err_code(id, mon[idx]);
}

static enum err_code node_mon_id_to_err_code(enum co_node_id id) {
  switch (id) {
    case CO_NODE_ID_FB:
      return ERR_CODE_NODE_FB;

    case CO_NODE_ID_RB:
      return ERR_CODE_NODE_RB;

    case CO_NODE_ID_ACC:
      return ERR_CODE_NODE_ACC;

    case CO_NODE_ID_INV_FL:
      return ERR_CODE_NODE_INV_FL;

    case CO_NODE_ID_INV_FR:
      return ERR_CODE_NODE_INV_FR;

    case CO_NODE_ID_INV_RL:
      return ERR_CODE_NODE_INV_RL;

    case CO_NODE_ID_INV_RR:
      return ERR_CODE_NODE_INV_RR;

    default:
      return 0;
  }
}

static void node_mon_update_err_code(enum co_node_id id, node_mon_t mon) {
  enum err_code err = node_mon_id_to_err_code(id);
  err_t errors = err_get_errors();

  if (mon && !(errors & err)) {
    err_set_errors(err, true);
  } else if (!mon && errors & err) {
    err_set_errors(err, false);
  }
}

#endif  // CO_CONFIG_HB_CONS

static int msg_emcy_init() {
  CO_EM_initCallbackRx(msg.co->CO->em, emcy_cb);

#if CO_CONFIG_HB_CONS & CO_CONFIG_HB_CONS_ENABLE
  CO_HBconsumer_t *HBcons = msg.co->CO->HBcons;
  for (int i = 0; i < HBcons->numberOfMonitoredNodes; i++) {
    CO_HBconsumer_initCallbackNmtChanged(HBcons, i, msg.mon, hb_mnt_changed_cb);
    CO_HBconsumer_initCallbackHeartbeatStarted(HBcons, i, msg.mon,
                                               hb_started_cb);
    CO_HBconsumer_initCallbackTimeout(HBcons, i, msg.mon, hb_timeout_cb);
    CO_HBconsumer_initCallbackRemoteReset(HBcons, i, msg.mon, hb_reset_cb);

    // set initial monitor node state to error
    uint32_t id;
    OD_get_u32(OD_ENTRY_H1016, i + 1, &id, true);
    id = (id >> 16) & 0xFF;

    enum err_code err = node_mon_id_to_err_code(id);
    if (err != 0) {
      err_set_errors(err, true);
    } else {
      LOG_WRN("No error code corresponds to node ID: %u at subindex: 0x%02X",
              id, i + 1);
    }
  }
#endif  // CO_CONFIG_HB_CONS

  return 0;
}

static int rpdo_od_init() {
  OD_entry_t *entry;
  STRUCT_SECTION_FOREACH(msg_rx_init, init) {
    if ((entry = OD_find(OD, init->idx)) == NULL) {
      LOG_ERR("OD entry not found: 0x%04X", init->idx);
      return -ENOENT;
    }

    OD_extension_init(entry, init->ext);
  }

  return 0;
}

static int tpdo_od_init() {
  OD_entry_t *entry;
  STRUCT_SECTION_FOREACH(msg_tx_init, init) {
    if ((entry = OD_find(OD, init->idx)) == NULL) {
      LOG_ERR("OD entry not found: 0x%04X", init->idx);
      return -ENOENT;
    }

    OD_extension_init(entry, &init->ext);
    init->ent = entry;
  }

  return 0;
}
