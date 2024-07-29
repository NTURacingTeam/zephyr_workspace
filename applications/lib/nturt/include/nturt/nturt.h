#ifndef NTURT_NTURT_H_
#define NTURT_NTURT_H_

// zephyr includes
#include <zephyr/input/input.h>

/// @brief System features.
#define FRONT_INVERTER 0
#define FRONT_SUSP 0

/// @brief Dashboard LED numbers.
#define LED_NUM_RTD 0
#define LED_NUM_RUNNING 4
#define LED_NUM_MODE 6
#define LED_NUM_PEDAL_PLAUS 8
#define LED_NUM_ERR 12

/// @brief Dashboard button keys.
#define INPUT_KEY_RTD INPUT_KEY_PLAY
#define INPUT_KEY_RESET INPUT_KEY_POWER
#define INPUT_KEY_CTRL_MODE INPUT_KEY_MENU
// INPUT_KEY_HOME already defined by zephyr

/// @brief Dashboard button numbers.
#define BTN_NUM_RTD 0
#define BTN_NUM_RESET 1
#define BTN_NUM_DIR 2
#define BTN_NUM_HOME 3

#endif  // NTURT_NTURT_H_
