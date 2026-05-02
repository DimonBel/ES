#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "lcd/lcd.h"
#include "dht11/dht11.h"
#include "actuator/actuator.h"
#include "joystick/joystick.h"
#include "led/led.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace freertos_app::internal {

struct SharedData {
    // Serial command interface
    bool serial_command_received;
    char serial_command_buffer[16];
    uint8_t serial_command_index;

    // Lab 5.1 Variant A – ON-OFF Temperature Control with Hysteresis
    float setpoint;      // target temperature in °C
    float temperature;   // measured temperature in °C
    bool  relayOn;       // current relay state
    float hysteresis;    // deadband in °C
};

// LCD pins
extern const uint8_t LCD_SDA_PIN;      // GPIO 21
extern const uint8_t LCD_SCL_PIN;      // GPIO 22

// Sensor pin
extern const uint8_t DHT11_PIN;        // GPIO 5

// Relay pin
extern const uint8_t RELAY_PIN;        // GPIO 23

// Input / status pins
extern const uint8_t JOYSTICK_SW_PIN;  // GPIO 18
extern const uint8_t LED_PIN;          // GPIO 2

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
extern const float DEFAULT_HYSTERESIS;
extern const float SETPOINT_MIN;
extern const float SETPOINT_MAX;
extern const float SETPOINT_STEP;

// Hardware objects
extern LcdI2c   *lcd;
extern Dht11Sensor *dht11Sensor;
extern Actuator *relay;
extern Joystick *joystick;
extern Led      *led;

// Synchronization
extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semControlDisplay;

// Shared data
extern SharedData sharedData;

}

#endif
