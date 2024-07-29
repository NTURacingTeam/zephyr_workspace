#ifndef NTURT_SENSORS_H_
#define NTURT_SENSORS_H_

// zephyr includes
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

// project includes
#include "nturt/err.h"

/* macro ---------------------------------------------------------------------*/
/**
 * @brief Desginated initializer for @ref sensor_tol.
 *
 * @param NAME Name of the sensor tolerant read structure.
 * @param NODE_ID Node ID of the sensor device.
 * @param TOL Tolerance, i.e. number of consecutive failed reads before setting
 * error.
 * @param ERR Error code corresponding to the sensor.
 */
#define SENSOR_TOL_DEFINE(NAME, NODE_ID, TOL, ERR) \
  struct sensor_tol NAME = {                       \
      .dev = DEVICE_DT_GET(NODE_ID),               \
      .tol = TOL,                                  \
      .err = ERR,                                  \
      .fail = 0,                                   \
  }

/* type ----------------------------------------------------------------------*/
/// @brief Sensor tolerant read structure.
struct sensor_tol {
  /// @brief Sensor device.
  const struct device *dev;

  /// @brief Tolerance, i.e. number of consecutive failed reads before setting
  /// error.
  const int tol;

  /// @brief Error code corresponding to the sensor.
  const enum err_code err;

  /// @brief Number of consecutive failed reads.
  int fail;
};

/* function declaration ------------------------------------------------------*/
/**
 * @brief Read sensor data with tolerance. Sets error if the sensor fails
 * consecutively. Does nothing if the sensor error is set and returns false.
 *
 * @param tol Sensor tolerant read structure.
 * @param chan Sensor channel.
 * @param val Sensor value.
 * @return 0 if read successful, negative error code otherwise.
 */
int sensor_tol_chan_read(struct sensor_tol *tol, enum sensor_channel chan,
                         struct sensor_value *val);

#endif  // NTURT_SENSORS_H_
