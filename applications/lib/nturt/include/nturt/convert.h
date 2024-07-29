#ifndef NTURT_CONVERT_H_
#define NTURT_CONVERT_H_

// glibc includes
#include <stdint.h>

/* Physical to CAN -----------------------------------------------------------*/
#define STEER_PHY_TO_CAN(PHY) (int16_t)(PHY)

/// @brief Pedal travel physical data in percentage (float), convert to 0.5%
/// (uint8_t).
#define PEDAL_TRAV_PHY_TO_CAN(PHY) (uint8_t)(2.0F * PHY)

/// @todo
#define APPS_RAW_PHY_TO_CAN(PHY) (int8_t)(PHY)

/// @todo
#define BSE_RAW_PHY_TO_CAN(PHY) (uint8_t)(PHY)

/// @brief Wheel speed physical data in RPM (uint16_t), convert to RPM
/// (uint16_t).
#define WHEEL_SPEED_PHY_TO_CAN(PHY) (uint16_t)(PHY)

/// @todo
#define SUSP_PHY_TO_CAN(PHY) (PHY)

/// @brief Inverter motor speed data in rad/s (float), convert to RPM (int16_t).
#define INV_SPEED_PHY_TO_CAN(PHY) (int16_t)(95.493F * PHY)

/// @brief Inverter torque data in m*s (float), convert to 0.001 m*s (int16_t).
#define INV_TORQUE_PHY_TO_CAN(PHY) (int16_t)(1000.0F * PHY)

/// @brief Inverter voltage data in V (float), convert to 0.01 V (uint16_t).
#define INV_VOLTAGE_PHY_TO_CAN(PHY) (uin16_t)(100.0F * PHY)

/// @brief Inverter current data in A (float), convert to 0.01 A (int16_t).
#define INV_CURRENT_PHY_TO_CAN(PHY) (int16_t)(100.0F * PHY)

/* CAN to physical -----------------------------------------------------------*/
/// @brief Wheel speed CAN data in RPM (uint16_t), convert to (float).
#define WHEEL_SPEED_CAN_TO_PHY(CAN) (float)(CAN)

/// @brief IMU CAN acceleration data in 0.001 g (int16_t), convert to m/s^2
/// (float).
#define IMU_ACCEL_CAN_TO_PHY(CAN) (0.00981F * CAN)

/// @brief IMU CAN gyro data in 0.1 deg/s (int16_t), convert to RPM
/// (float).
#define IMU_GYRO_CAN_TO_PHY(CAN) (CAN / 60.0F)

/// @brief Inverter motor speed data in RPM (int16_t), convert to (float).
#define INV_SPEED_CAN_TO_PHY(CAN) (float)(CAN)

/// @brief Inverter torque data in 0.001 m*s (int16_t), convert to m*s (float).
#define INV_TORQUE_CAN_TO_PHY(CAN) (0.001F * CAN)

/// @brief Inverter voltage data in 0.01 V (uint16_t), convert to V (float).
#define INV_VOLTAGE_CAN_TO_PHY(CAN) (0.01F * CAN)

/// @brief Inverter current data in 0.01 A (int16_t), convert to A (float).
#define INV_CURRENT_CAN_TO_PHY(CAN) (0.01F * CAN)

#endif  // NTURT_CONVERT_H_
