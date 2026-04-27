#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "lcd/lcd.h"
#include "motor/motor.h"
#include "joystick/joystick.h"
#include "led/led.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"
#include "potentiometer/potentiometer.h"

namespace freertos_app::internal {

struct SharedData {
    // Serial command interface
    bool serial_command_received;
    char serial_command_buffer[16];
    uint8_t serial_command_index;

    // Lab 6.2.1 – ON-OFF Motor Control with Hysteresis
    int setpoint;       // 0-100 % from potentiometer
    int value;          // 0-100 % from joystick X (simulated position sensor)
    int output;         // -1 = BACKWARD, 0 = STOP, 1 = FORWARD
    int hysteresis;     // deadband width in % (default 5)
    int motorSpeed;     // fixed saturation speed in % (default 50)
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;
extern const uint8_t LCD_SCL_PIN;

// Motor driver pins (L298N)
extern const uint8_t MOTOR_IN1_PIN;   // GPIO 25
extern const uint8_t MOTOR_IN2_PIN;   // GPIO 26
extern const uint8_t MOTOR_ENA_PIN;   // GPIO 27 – PWM

// Sensor pins
extern const uint8_t POTENTIOMETER_PIN;   // GPIO 34 – SetPoint
extern const uint8_t JOYSTICK_X_PIN;      // GPIO 35 – Value (position)
extern const uint8_t JOYSTICK_Y_PIN;      // GPIO 32
extern const uint8_t JOYSTICK_SW_PIN;     // GPIO 0  (BOOT)
extern const uint8_t LED_PIN;             // GPIO 2  (built-in, status)

// Task configuration
extern const uint32_t TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_ACQUISITION;
extern const UBaseType_t TASK_PRIORITY_CONTROL;

// Task periods
extern const uint32_t ACQUISITION_PERIOD_MS;
extern const uint32_t CONTROL_PERIOD_MS;
extern const uint32_t DISPLAY_PERIOD_MS;

// Control constants
extern const int MOTOR_SATURATION_SPEED;
extern const int DEFAULT_HYSTERESIS;

// Hardware objects
extern LcdI2c       *lcd;
extern Motor        *motor;
extern Joystick     *joystick;
extern Led          *led;
extern Potentiometer *potentiometer;

// Synchronization
extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semControlDisplay;

// Shared data
extern SharedData sharedData;

}

#endif
