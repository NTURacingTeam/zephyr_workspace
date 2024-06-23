#define DT_DRV_COMPAT cui_amt21

// glibc includes
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// zephyr include
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(amt21, CONFIG_SENSOR_LOG_LEVEL);

#define AMT21_RECV_TIMEOUT K_MSEC(5)

#define AMT21_ANG_RES 14

#define AMT21_CMD_READ_POS 0x0
#define AMT21_CMD_READ_TURNS 0x1
#define AMT21_CMD_EXT 0x2

#define AMT21_EXT_CMD_SET_ZERO 0x5E

struct amt21_data {
  struct sensor_value val;
};

struct amt21_config {
  const struct device* uart;

  uint8_t addr;

  bool multi_turn;
};

static int uart_poll_in_timeout(const struct device* dev, uint8_t* c,
                                k_timeout_t timeout) {
  k_timepoint_t end = sys_timepoint_calc(timeout);

  do {
    int ret = uart_poll_in(dev, c);
    if (ret != -1) {
      return ret;
    }

    timeout = sys_timepoint_timeout(end);
  } while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));

  return -ETIMEDOUT;
}

static bool check_and_remove_checksum(uint16_t* data) {
  bool even = false;
  bool odd = false;
  for (int i = 0; i < 8; i++) {
    even ^= (*data >> (2 * i)) & 0x1;
    odd ^= (*data >> (2 * i + 1)) & 0x1;
  }

  *data &= 0x3FFF;

  return even && odd;
}

static int amt21_init(const struct device* dev) {
  const struct amt21_config* config = dev->config;

  if (!device_is_ready(config->uart)) {
    LOG_ERR("UART bus %s is not ready", config->uart->name);
    return -ENODEV;
  }

  return 0;
}

static int amt21_sample_fetch(const struct device* dev,
                              enum sensor_channel chan) {
  if (k_is_in_isr()) {
    return -EWOULDBLOCK;
  }

  if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION) {
    return -ENOTSUP;
  }

  int ret = 0;
  struct amt21_data* data = dev->data;
  const struct amt21_config* config = dev->config;

  int16_t turns = 0;
  uint16_t count = 0;
  uint8_t buf[2];

  if (config->multi_turn) {
    uart_poll_out(config->uart, config->addr + AMT21_CMD_READ_TURNS);
    for (int j = 0; j < sizeof(buf); j++) {
      ret = uart_poll_in_timeout(config->uart, &buf[j], AMT21_RECV_TIMEOUT);
      if (ret < 0) {
        goto err;
      }
    }

    uint16_t raw = sys_get_le16(buf);
    if (!check_and_remove_checksum(&raw)) {
      goto err_checksum;
    }

    turns = raw;

    // sign extend from 14 bits to 16 bits
    if (turns & 0x2000) {
      turns |= 0xC000;
    }
  }

  uart_poll_out(config->uart, config->addr + AMT21_CMD_READ_POS);
  for (int i = 0; i < sizeof(buf); i++) {
    ret = uart_poll_in_timeout(config->uart, &buf[i], AMT21_RECV_TIMEOUT);
    if (ret < 0) {
      goto err;
    }
  }

  count = sys_get_le16(buf);
  if (!check_and_remove_checksum(&count)) {
    goto err_checksum;
  }

  int64_t conv = (int64_t)turns * 1000000 * 360 +
                 (((int64_t)count * 1000000 * 360) >> AMT21_ANG_RES);
  sensor_value_from_micro(&data->val, conv);

  return 0;

err:
  LOG_ERR("Failed to receive from device: %s", strerror(-ret));
  return ret;

err_checksum:
  LOG_WRN("Checksum error");
  return -EIO;
}

static int amt21_channel_get(const struct device* dev, enum sensor_channel chan,
                             struct sensor_value* val) {
  if (chan != SENSOR_CHAN_ROTATION) {
    return -ENOTSUP;
  }

  struct amt21_data* data = dev->data;
  *val = data->val;

  return 0;
}

static int amt21_attribute_set(const struct device* dev,
                               enum sensor_channel chan,
                               enum sensor_attribute attr,
                               const struct sensor_value* val) {
  if (chan != SENSOR_CHAN_ROTATION || attr != SENSOR_ATTR_OFFSET) {
    return -ENOTSUP;
  }

  struct amt21_data* data = dev->data;
  const struct amt21_config* config = dev->config;

  if (val->val1 != -data->val.val1 || val->val2 != -data->val.val2) {
    LOG_ERR(
        "Only supports setting offset to the negative of the current value, "
        "i.e. setting zero "
        "position");
    return -ENOTSUP;
  }

  uart_poll_out(config->uart, config->addr + AMT21_CMD_EXT);
  uart_poll_out(config->uart, AMT21_EXT_CMD_SET_ZERO);

  return 0;
}

static const struct sensor_driver_api amt21_api = {
    .sample_fetch = amt21_sample_fetch,
    .channel_get = amt21_channel_get,
    .attr_set = amt21_attribute_set,
};

#define AMT21_INIT(inst)                                             \
  static struct amt21_data amt21_data_##inst = {                     \
      .val = {0},                                                    \
  };                                                                 \
                                                                     \
  static const struct amt21_config amt21_config_##inst = {           \
      .uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                      \
      .addr = DT_INST_PROP(inst, addr),                              \
      .multi_turn = DT_INST_PROP(inst, multi_turn),                  \
  };                                                                 \
                                                                     \
  DEVICE_DT_INST_DEFINE(inst, &amt21_init, NULL, &amt21_data_##inst, \
                        &amt21_config_##inst, POST_KERNEL,           \
                        CONFIG_SENSOR_INIT_PRIORITY, &amt21_api);

DT_INST_FOREACH_STATUS_OKAY(AMT21_INIT)
