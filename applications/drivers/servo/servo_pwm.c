#define DT_DRV_COMPAT pwm_servo

// glibc include
#include <math.h>
#include <stdint.h>

// zephyr include
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/servo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(servo_pwm, CONFIG_SERVO_LOG_LEVEL);

#define SERVO_PWM_DUTY_NOT_SET -123456789

struct servo_pwm_config {
  const struct pwm_dt_spec pwm;

  const uint32_t min_pulse;

  const uint32_t neutral_pulse;

  const uint32_t max_pulse;

  const int initial_duty;
};

static uint32_t map(const struct servo_pwm_config *config, int duty) {
  __ASSERT(duty <= SERVO_MAX_DUTY && duty >= SERVO_MIN_DUTY,
           "Invalid duty cycle %d", duty);

  if (duty >= 0) {
    return config->neutral_pulse +
           (config->max_pulse - config->neutral_pulse) * duty / SERVO_MAX_DUTY;
  } else {
    // avoid negative values for unsigned types
    return config->neutral_pulse - (config->neutral_pulse - config->min_pulse) *
                                       (-duty) / SERVO_MAX_DUTY;
  }
}

static int servo_pwm_set(const struct device *dev, int duty) {
  const struct servo_pwm_config *config = dev->config;

  return pwm_set_pulse_dt(&config->pwm, map(config, duty));
}

static const struct servo_driver_api servo_api = {
    .set = servo_pwm_set,
};

static int servo_init(const struct device *dev) {
  const struct servo_pwm_config *config = dev->config;

  if (config->initial_duty == SERVO_PWM_DUTY_NOT_SET) {
    return 0;
  }

  __ASSERT(config->initial_duty <= SERVO_MAX_DUTY &&
               config->initial_duty >= SERVO_MIN_DUTY,
           "Invalid initial duty cycle %d", config->initial_duty);

  return pwm_set_pulse_dt(&config->pwm, map(config, config->initial_duty));
}

#define SERVO_INIT(inst)                                                    \
  static const struct servo_pwm_config servo_pwm_config_##inst = {          \
      .pwm = PWM_DT_SPEC_INST_GET(inst),                                    \
      .min_pulse = DT_INST_PROP(inst, min_pulse),                           \
      .neutral_pulse = DT_INST_PROP_OR(                                     \
          inst, neutral_pulse,                                              \
          (DT_INST_PROP(inst, min_pulse) + DT_INST_PROP(inst, max_pulse)) / \
              2),                                                           \
      .max_pulse = DT_INST_PROP(inst, max_pulse),                           \
      .initial_duty =                                                       \
          DT_INST_PROP_OR(inst, initial_duty, SERVO_PWM_DUTY_NOT_SET),      \
  };                                                                        \
                                                                            \
  DEVICE_DT_INST_DEFINE(inst, servo_init, NULL, NULL,                       \
                        &servo_pwm_config_##inst, POST_KERNEL,              \
                        CONFIG_SERVO_INIT_PRIORITY, &servo_api);

DT_INST_FOREACH_STATUS_OKAY(SERVO_INIT)
