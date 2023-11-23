/**
 * @file servo.h
 * @author QuantumSpawner jet22854111@gmail.com
 * @brief
 *
 */

#ifndef ZEPHYR_DRIVERS_SERVO_H_
#define ZEPHYR_DRIVERS_SERVO_H_

// glibc include
#include <errno.h>
#include <stdint.h>

// zephyr include
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_MAX_DUTY 100
#define SERVO_MIN_DUTY -100

typedef int (*servo_set_t)(const struct device *dev, int duty);

struct servo_driver_api {
  servo_set_t set;
};

/**
 * @brief Set servo duty cycle.
 *
 * @param[in] dev Servo device instance.
 * @param[in] duty Duty cycle from @ref SERVO_MIN_DUTY to @ref SERVO_MAX_DUTY.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
__syscall int servo_set(const struct device *dev, int duty);

static inline int z_impl_servo_set(const struct device *dev, int duty) {
  const struct servo_driver_api *api =
      (const struct servo_driver_api *)dev->api;

  if (!api->set) {
    return -ENOSYS;
  }

  return api->set(dev, duty);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/servo.h>

#endif  // ZEPHYR_DRIVERS_SERVO_H_
