#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "lcd/lcd.h"
#include "dht11/dht11.h"
#include "motor/motor.h"
#include "joystick/joystick.h"
#include "led/led.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace freertos_app::internal {

struct SharedData {
    // Serial command interface
    bool    serial_command_received;
    char    serial_command_buffer[16];
    uint8_t serial_command_index;

    // Lab 5.2 Variant A – PID Temperature Control (DHT11 + L298N)
    float setpoint;      // target temperature °C
    float temperature;   // measured temperature °C
    float pidOutput;     // 0–100 %

    // PID state
    float kp, ki, kd;
    float integral;
    float prevError;
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;   // GPIO 21
extern const uint8_t LCD_SCL_PIN;   // GPIO 22

// DHT11 sensor pin
extern const uint8_t DHT11_PIN;     // GPIO 5

// L298N motor driver pins
extern const uint8_t MOTOR_IN1_PIN; // GPIO 25
extern const uint8_t MOTOR_IN2_PIN; // GPIO 26
extern const uint8_t MOTOR_ENA_PIN; // GPIO 27 (PWM)

// Button / LED
extern const uint8_t JOYSTICK_SW_PIN; // GPIO 18
extern const uint8_t LED_PIN;         // GPIO 2

// Task configuration
extern const uint32_t    TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_ACQUISITION;
extern const UBaseType_t TASK_PRIORITY_CONTROL;

// Task periods
extern const uint32_t ACQUISITION_PERIOD_MS;
extern const uint32_t CONTROL_PERIOD_MS;
extern const uint32_t DISPLAY_PERIOD_MS;

// Control constants
extern const float DEFAULT_SETPOINT;
extern const float SETPOINT_MIN;
extern const float SETPOINT_MAX;
extern const float SETPOINT_STEP;
extern const float DEFAULT_KP;
extern const float DEFAULT_KI;
extern const float DEFAULT_KD;

// Hardware objects
extern LcdI2c      *lcd;
extern Dht11Sensor *dht11Sensor;
extern Motor       *motor;
extern Joystick    *joystick;
extern Led         *led;

// Synchronization
extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semControlDisplay;

// Shared data
extern SharedData sharedData;

}

#endif
