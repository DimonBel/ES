#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "lcd/lcd.h"
#include "actuator/actuator.h"
#include "signal_conditioner/signal_conditioner.h"
#include "joystick/joystick.h"
#include "led/led.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace freertos_app::internal {

struct SharedData {
    // Actuator control data (Lab 4.1)
    bool actuator_command;        // Command from user (ON/OFF)
    bool actuator_state;          // Current actuator state
    bool actuator_conditioned;    // Conditioned signal state
    uint32_t actuator_command_time;
    uint32_t actuator_toggle_count;
    bool serial_command_received;
    char serial_command_buffer[16];
    uint8_t serial_command_index;
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;
extern const uint8_t LCD_SCL_PIN;

// Actuator and Button pins (Lab 4.1)
extern const uint8_t ACTUATOR_PIN;
extern const uint8_t BUTTON_PIN;

// Additional UI pins
extern const uint8_t JOYSTICK_X_PIN;
extern const uint8_t JOYSTICK_Y_PIN;
extern const uint8_t JOYSTICK_SW_PIN;
extern const uint8_t LED_PIN;

// Task configuration
extern const uint32_t TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_ACTUATOR;    // Lab 4.1
extern const UBaseType_t TASK_PRIORITY_CONDITIONING; // Lab 4.1

// Actuator control configuration (Lab 4.1)
extern const uint32_t ACTUATOR_CONTROL_PERIOD_MS;
extern const uint32_t ACTUATOR_DEBOUNCE_TIME_MS;
extern const uint32_t ACTUATOR_VALIDATION_TIME_MS;
extern const uint32_t DISPLAY_PERIOD_MS;

// Component pointers
extern LcdI2c *lcd;
extern Actuator *actuator;          // Lab 4.1
extern SignalConditioner *signalConditioner; // Lab 4.1
extern Joystick *joystick;
extern Led *led;

// Synchronization primitives
extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semActuatorDisplay; // Lab 4.1

// Shared data
extern SharedData sharedData;

}

#endif