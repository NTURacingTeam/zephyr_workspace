#ifndef ZEPHYR_DRIVERS_SENSOR_JY901B_JY901B_H_
#define ZEPHYR_DRIVERS_SENSOR_JY901B_JY901B_H_

// glibc include
#include <stdint.h>

// zephyr include
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>


// i2c register
#define JY901B_DATA_REG_START 0x34
#define JY901B_DATA_LENGTH 26

#define JY901B_PRESSURE_REG 0x45
#define JY901B_PRESSURE_LENGTH 4

#define JY901B_QUAT_REG 0x51
#define JY901B_QUAT_LENGTH 8

#define JY901B_I2C_ADDR_REG 0x1A

#define JY901B_ORIEN_MODE_REG 0x24
#define JY901B_ORIEN_MODE_LENGTH 2
#define JY901B_ORIEN_MODE_ABSOLUTE 0x0000
#define JY901B_ORIEN_MODE_RELATIVE 0x0001

#define JY901B_WRITE_ENABLE_REG 0x69
#define JY901B_WRITE_ENABLE_LENGTH 2
#define JY901B_WRITE_ENABLE_KEY 0xB588

// data scale
#define JY901B_DATA_SHIFT 15

#define JY901B_ACCEL_SCALE 16
#define JY901B_GYRO_SCALE 2000
#define JY901B_MAGN_SCALE 15000
#define JY901B_DEG_SCALE 180
#define JY901B_QUAT_SCALE (1 / 1000)

#define JY901B_TEMP_SCALE 100
#define JY901B_PRESS_SCALE (1 / 1000)

enum jy901b_channel {
  /// @brief Orientation in degrees.
  SENSOR_CHAN_ORIEN_RPY = SENSOR_CHAN_PRIV_START,
  SENSOR_CHAN_ORIEN_ROLL,
  SENSOR_CHAN_ORIEN_PITCH,
  SENSOR_CHAN_ORIEN_YAW,

  /// @brief Quaternion in 1 / 1000.
  SENSOR_CHAN_QUAT_WXYZ,
  SENSOR_CHAN_QUAT_W,
  SENSOR_CHAN_QUAT_X,
  SENSOR_CHAN_QUAT_Y,
  SENSOR_CHAN_QUAT_Z,
};

struct jy901b_data {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;

  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;

  int16_t magn_x;
  int16_t magn_y;
  int16_t magn_z;

  int16_t roll;
  int16_t pitch;
  int16_t yaw;

  int16_t quat_w;
  int16_t quat_x;
  int16_t quat_y;
  int16_t quat_z;

  int16_t temp;
  int32_t press;
};

struct jy901b_config {
  const struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_JY901B_JY901B_H_ */
