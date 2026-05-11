#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "lcd/lcd.h"
#include "joystick/joystick.h"
#include "led/led.h"
#include "kernel_primitives/mutex/mutex.h"

namespace freertos_app::internal {

enum class LedState { OFF, ON };

struct SharedData {
    LedState ledState;
    uint32_t lastPressTime;  // millis() of last accepted press (debounce)
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;    // GPIO 21
extern const uint8_t LCD_SCL_PIN;    // GPIO 22

// Button / LED pins
extern const uint8_t JOYSTICK_SW_PIN; // GPIO 17
extern const uint8_t LED_PIN;         // GPIO 2

// Task configuration
extern const uint32_t    TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_FSM;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;

// Task periods / timing
extern const uint32_t FSM_PERIOD_MS;
extern const uint32_t DISPLAY_PERIOD_MS;
extern const uint32_t DEBOUNCE_MS;

// Hardware objects
extern LcdI2c   *lcd;
extern Joystick *joystick;
extern Led      *led;

// Synchronization
extern kernel_primitives::Mutex lcdMutex;

// Shared data
extern SharedData sharedData;

}

#endif
