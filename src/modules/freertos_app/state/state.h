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
#include "servo/servo.h"
#include "potentiometer/potentiometer.h"

namespace freertos_app::internal {

struct SharedData {
    // Actuator control data (Lab 4.1 - Binary Actuator)
    bool actuator_command;        // Command from user (ON/OFF)
    bool actuator_state;          // Current actuator state
    bool actuator_conditioned;    // Conditioned signal state
    uint32_t actuator_command_time;
    uint32_t actuator_toggle_count;
    bool serial_command_received;
    char serial_command_buffer[16];
    uint8_t serial_command_index;

    // Servo control data (Lab 4.2 - Analog Actuator)
    int servo_speed;              // Speed/position (0-100%)
    int servo_angle;              // Current angle in degrees (0-180)
    int potentiometer_raw;        // Raw ADC value (0-4095)
    int potentiometer_percent;    // Potentiometer value in percent (0-100)
    bool servo_enabled;           // Servo control enabled flag
    uint32_t servo_command_time;
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;
extern const uint8_t LCD_SCL_PIN;

// Actuator and Button pins (Lab 4.1 - Binary)
extern const uint8_t ACTUATOR_PIN;          // Relay control (GPIO 23)
extern const uint8_t BUTTON_PIN;            // Manual toggle button (GPIO 19)

// Servo and Potentiometer pins (Lab 4.2 - Analog)
extern const uint8_t SERVO_PIN;             // Servo PWM signal (GPIO 18)
extern const uint8_t POTENTIOMETER_PIN;     // Analog input for speed control (GPIO 34)

// Additional UI pins (reconfigured to avoid conflicts)
extern const uint8_t JOYSTICK_X_PIN;        // Joystick X-axis (GPIO 35)
extern const uint8_t JOYSTICK_Y_PIN;        // Joystick Y-axis (GPIO 32)
extern const uint8_t JOYSTICK_SW_PIN;       // Joystick button (GPIO 0 = BOOT button)
extern const uint8_t LED_PIN;               // Status LED (GPIO 26)

// Task configuration
extern const uint32_t TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_ACTUATOR;      // Lab 4.1 - Binary actuator control
extern const UBaseType_t TASK_PRIORITY_CONDITIONING; // Lab 4.1 - Signal conditioning
extern const UBaseType_t TASK_PRIORITY_SERVO;        // Lab 4.2 - Servo control (Priority 3)

// Actuator control configuration (Lab 4.1)
extern const uint32_t ACTUATOR_CONTROL_PERIOD_MS;
extern const uint32_t ACTUATOR_DEBOUNCE_TIME_MS;
extern const uint32_t ACTUATOR_VALIDATION_TIME_MS;
extern const uint32_t DISPLAY_PERIOD_MS;

// Servo control configuration (Lab 4.2)
extern const uint32_t SERVO_CONTROL_PERIOD_MS;       // Servo control period (50ms)
extern const uint32_t SERVO_COOLDOWN_MS;             // Cooldown between servo commands (250ms)

// Component pointers
extern LcdI2c *lcd;
extern Actuator *actuator;          // Lab 4.1 - Binary actuator (relay)
extern SignalConditioner *signalConditioner; // Lab 4.1 - Signal conditioning
extern Joystick *joystick;
extern Led *led;
extern Servo *servo;                // Lab 4.2 - Analog actuator (servo)
extern Potentiometer *potentiometer; // Lab 4.2 - Analog input (speed control)

// Synchronization primitives
extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semActuatorDisplay; // Lab 4.1

// Shared data
extern SharedData sharedData;

}

#endif