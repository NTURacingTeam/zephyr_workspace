#ifndef NTURT_MSG_H_
#define NTURT_MSG_H_

// glibc include
#include <string.h>

// zephyr includes
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

// lib includes
#include <canopennode.h>

// project includes
#include "nturt/err.h"
#include "nturt/util.h"

/**
 * @addtogroup Msg Message
 *
 * @{
 */

/* macro ---------------------------------------------------------------------*/
/**
 * @addtogroup MsgMacro Message Macros
 *
 * @{
 */

/// @brief CAN bus baudrate of the network.
#define MSG_CAN_BAUDRATE 500

/**
 * @addtogroup NodeMonMacro Node Monitor Macros
 *
 * @{
 */

/// @brief Node monitor states.
#define NODE_MON_ERR BIT(0)

#define NODE_MON_NOT_OPT BIT(1)

#define NODE_MON_TIMEOUT BIT(2)

/// @brief Initial state of monitored node.
#define NODE_MON_INITIAL (NODE_MON_NOT_OPT | NODE_MON_TIMEOUT)

/**
 * @} // NodeMonMacro
 */

/**
 * @addtogroup MsgRxMacro Message reception macros
 *
 * @{
 */

/**
 * @brief Define a RPDO reception variable.
 *
 * @param TYPE Type of the variable, must be the same as defined in the object
 * dictionary.
 * @param NAME Name of the variable, does not need to be the same as defined in
 * the object dictionary.
 */
#define RX_VAR(TYPE, NAME) TYPE, NAME

#define _RX_GROUP_VAR(TYPE, NAME) TYPE NAME

#define _RX_GROUP_ENUM(IDX, TYPE, NAME, GROUP) __##GROUP##_##NAME = BIT(IDX)

/**
 * @brief Group multiple RPDO reception variables together into a struct. Calls
 * a callback function when all variables in the group are received.
 *
 * @param NAME Name of the RPDO reception group.
 * @param CB Callback function to call when all variables in the group are
 * received, it must have the signature void cb(const void *data) where data is
 * a pointer to the group.
 */
#define RX_GROUP(NAME, CB, ...)                                           \
  struct NAME {                                                           \
    FOR_EACH_PAIR(_RX_GROUP_VAR, (;), __VA_ARGS__);                       \
    uint32_t __updated;                                                   \
    void (*__cb)(const void *);                                           \
  };                                                                      \
                                                                          \
  enum {                                                                  \
    FOR_EACH_PAIR_IDX_FIXED_ARG(_RX_GROUP_ENUM, (, ), NAME, __VA_ARGS__), \
    __##NAME##_mask = BIT_MASK(NUM_VA_ARGS(__VA_ARGS__) / 2),             \
  };                                                                      \
                                                                          \
  static struct NAME __##NAME = {                                         \
      .__updated = 0,                                                     \
      .__cb = CB,                                                         \
  }

/**
 * @brief Declare the subindex of a RPDO reception variable in the object
 * dictionary.
 *
 * @param NAME Name of the reception variable.
 * @param SUBIDX Subindex of the reception variable in the object dictionary
 * entry.
 */
#define RX_OD_VAR(NAME, SUBIDX) NAME, SUBIDX

#define _RX_OD_WRITE_CASE(VAR, SUBIDX, GROUP)                 \
  case SUBIDX:                                                \
    memcpy(&dest->VAR, stream->dataOrig, stream->dataLength); \
    dest->__updated |= __##GROUP##_##VAR;                     \
    break

/**
 * @brief Define the write function for a object dictionary extension.
 *
 * @param NAME Name of the function.
 * @param GROUP Group of RPDO reception variables defined by @ref RX_GROUP.
 * @param ... List of RPDO reception variables defined by @ref RX_OD_VAR.
 */
#define RX_OD_WRITE_DEFINE(NAME, GROUP, ...)                               \
  static ODR_t NAME(OD_stream_t *stream, const void *buf, OD_size_t size,  \
                    OD_size_t *size_written) {                             \
    if (stream == NULL || buf == NULL || size_written == NULL) {           \
      return ODR_DEV_INCOMPAT;                                             \
    }                                                                      \
                                                                           \
    ODR_t ret = OD_writeOriginal(stream, buf, size, size_written);         \
                                                                           \
    if (ret != ODR_OK) {                                                   \
      return ret;                                                          \
    }                                                                      \
                                                                           \
    struct GROUP *dest = stream->object;                                   \
    switch (stream->subIndex) {                                            \
      FOR_EACH_PAIR_FIXED_ARG(_RX_OD_WRITE_CASE, (;), GROUP, __VA_ARGS__); \
                                                                           \
      default:                                                             \
        return ODR_OK;                                                     \
    }                                                                      \
                                                                           \
    if ((dest->__updated & __##GROUP##_mask) == __##GROUP##_mask) {        \
      dest->__cb(dest);                                                    \
      dest->__updated = 0;                                                 \
    }                                                                      \
                                                                           \
    return ODR_OK;                                                         \
  }

/**
 * @brief Bind variables in a object dictionary entry to a RPDO reception group.
 * When the bound variables are received, it is updated to the group.
 *
 * @param IDX Index of the object dictionary entry to bind the group to.
 * @param GROUP Group of RPDO reception variables defined by @ref RX_GROUP.
 * @param ... List of RPDO reception variables in the group defined by @ref
 * RX_OD_VAR to be bound to the object dictionary entry.
 */
#define RX_OD_BIND(IDX, GROUP, ...)                             \
  RX_OD_WRITE_DEFINE(__od_write_##IDX, GROUP, __VA_ARGS__)      \
                                                                \
  static OD_extension_t __od_ext_##IDX = {                      \
      .object = &__##GROUP,                                     \
      .read = OD_readOriginal,                                  \
      .write = __od_write_##IDX,                                \
  };                                                            \
                                                                \
  STRUCT_SECTION_ITERABLE(msg_rx_init, __msg_rx_init_##IDX) = { \
      .idx = IDX,                                               \
      .ext = &__od_ext_##IDX,                                   \
  }

/**
 * @} // MsgRxMacro
 */

/**
 * @addtogroup MsgTxMacro Message transmission macros
 *
 * @{
 */

#define _TX_OD_DECLARE(IDX)                                     \
  STRUCT_SECTION_ITERABLE(msg_tx_init, __msg_tx_init_##IDX) = { \
      .idx = IDX,                                               \
      .ext =                                                    \
          {                                                     \
              .read = OD_readOriginal,                          \
              .write = OD_writeOriginal,                        \
          },                                                    \
  }

/**
 * @breif Declare the object dictionary entries for TPDO transmission.
 *
 * @param ... List of object dictionary indexes to be transmitted using TPDO.
 */
#define TX_OD_DECLARE(...) FOR_EACH(_TX_OD_DECLARE, (;), __VA_ARGS__)

/**
 * @brief Set the value of a object dictionary entry.
 *
 * @param IDX Index of the object dictionary entry.
 * @param SUBIDX Subindex of the object dictionary entry.
 * @param VALUE Value to set the object dictionary entry to.
 * @param TYPE Type of the object dictionary entry.
 */
#define TX_OD_SET(IDX, SUBIDX, VALUE, TYPE) \
  OD_set_##TYPE(__msg_tx_init_##IDX.ent, SUBIDX, VALUE, true)

/**
 * @brief Set the value of a object dictionary entry and send the TPDO.
 *
 * @param IDX Index of the object dictionary entry.
 * @param SUBIDX Subindex of the object dictionary entry.
 * @param VALUE Value to set the object dictionary entry to.
 * @param TYPE Type of the object dictionary entry.
 */
#define TX_OD_SEND(IDX, SUBIDX, VALUE, TYPE) \
  TX_OD_SET(IDX, SUBIDX, VALUE, TYPE);       \
  OD_requestTPDO(OD_getFlagsPDO(__msg_tx_init_##IDX.ent), SUBIDX)

/**
 * @} // MsgTxMacro
 */

/**
 * @} // MsgMacro
 */

/**
 * @addtogroup ErrMacro
 *
 * @{
 */

/**
 * @addtogroup ErrMacroCO CANopen Error Macros
 *
 * @{
 */

/// @brief Prefix for CANopen error code names.
#define ERR_CO_ERR_CODE_PREFIX CO_EMC_

/// @brief Prefix for CANopenNode error status bit names.
#define ERR_CO_ERR_STATUS_PREFIX CO_EM_

#define _ERR_CO_ERR_CODE_DECLARE(I, X, _) \
  CONCAT(ERR_CO_ERR_CODE_PREFIX, X) = CO_EMC_DEVICE_SPECIFIC + I

/**
 * @brief Declare the values of CANopen error codes using enum.
 *
 * @note The values for the error codes are 0xFF00 (CANopen device specific) +
 * the index of the error code in the list.
 *
 * @param LIST List of error codes defined by @ref ERROR_CODE_LISTIFY.
 * @return Enum of CANopen error codes.
 */
#define ERR_CO_ERR_CODE_DECLARE(LIST)                       \
  enum CO_EM_error_code {                                   \
    FOR_EACH_PAIR_IDX(_ERR_CO_ERR_CODE_DECLARE, (, ), LIST) \
  }

#define _ERR_CO_ERR_STATUS_DECLARE(I, X, _) \
  CONCAT(ERR_CO_ERR_STATUS_PREFIX, X) = CO_EM_MANUFACTURER_START + I

/**
 * @brief Declare the values of CANopenNode error status bits using enum.
 *
 * @note The values for the error status bits are 0x30
 * (CO_EM_MANUFACTURER_START) + the index of the error code in the list.
 *
 * @param LIST List of error codes defined by @ref ERROR_CODE_LISTIFY.
 * @return Enum of CANopenNode error status bits.
 */
#define ERR_CO_ERR_STATUS_DECLARE(LIST)                       \
  enum CO_EM_status_bit {                                     \
    FOR_EACH_PAIR_IDX(_ERR_CO_ERR_STATUS_DECLARE, (, ), LIST) \
  }

/**
 * @brief Convert the value of error code to the value of CANopen error code.
 *
 * @param ERR Error code.
 * @return CANopen error code.
 */
#define ERR_CODE_TO_CO_ERR_CODE(ERR) \
  (__builtin_ffs(ERR) + CO_EMC_DEVICE_SPECIFIC)

/**
 * @brief Convert the value of CANopen error code to the value of error code.
 *
 * @param ERR CANopen error code.
 * @return Error code.
 */
#define ERR_CO_ERR_CODE_TO_CODE(ERR) BIT(ERR - CO_EMC_DEVICE_SPECIFIC)

/**
 * @brief Convert the value of error code to the value of CANopenNode error
 * status bit.
 *
 * @param ERR Error code.
 * @return CANopenNode error status bit.
 */
#define ERR_CODE_TO_CO_ERR_STATUS(ERR) \
  (__builtin_ffs(ERR) + CO_EM_MANUFACTURER_START)

/**
 * @brief Convert the value of CANopenNode error status bit to the value of
 * error code.
 *
 * @param ERR CANopenNode error status bit.
 * @return Error code.
 */
#define ERR_CO_ERR_STATUS_TO_CODE(ERR) BIT(ERR - CO_EM_MANUFACTURER_START)

/**
 * @} // ErrMacroCO
 */

/**
 * @} // ErrMacro
 */

/* type ----------------------------------------------------------------------*/
/// @brief CANopen node ID.
enum co_node_id {
  CO_NODE_ID_RB = 0x01,
  CO_NODE_ID_FB,

  CO_NODE_ID_RPI = 0x04,

  CO_NODE_ID_IMU = 0x08,

  CO_NODE_ID_ACC = 0x10,
  CO_NODE_ID_INV_FL,
  CO_NODE_ID_INV_FR,
  CO_NODE_ID_INV_RL,
  CO_NODE_ID_INV_RR,
};

/// @brief Initialization structure for RPDO.
struct msg_rx_init {
  uint16_t idx;
  OD_extension_t *ext;
};

/// @brief Initialization structure for TPDO.
struct msg_tx_init {
  uint16_t idx;
  OD_extension_t ext;
  OD_entry_t *ent;
};

/// @brief Node monitor state.
typedef uint8_t node_mon_t;

ERR_CO_ERR_CODE_DECLARE(ERR_CODE_LIST);

ERR_CO_ERR_STATUS_DECLARE(ERR_CODE_LIST);

/* function declaration ------------------------------------------------------*/
/// @brief Initialization function for message module.
int msg_init(struct canopen *co);

int sdo_write(struct canopen *co, int node_id, int index, int subindex,
              const void *data, size_t size, int timeout);

/**
 * @} // Msg
 */

#endif  // NTURT_MSG_H_
