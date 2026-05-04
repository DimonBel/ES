#include "freertos_app/state/state.h"

namespace freertos_app::internal {

// LCD
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// DHT11
const uint8_t DHT11_PIN = 5;

// L298N
const uint8_t MOTOR_IN1_PIN = 25;
const uint8_t MOTOR_IN2_PIN = 26;
const uint8_t MOTOR_ENA_PIN = 27;

// Button / LED
const uint8_t JOYSTICK_SW_PIN = 18;
const uint8_t LED_PIN         = 2;

// Task configuration
const uint32_t    TASK_STACK_SIZE           = 4096;
const UBaseType_t TASK_PRIORITY_DISPLAY     = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_ACQUISITION = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_CONTROL     = tskIDLE_PRIORITY + 3;

// Task periods
const uint32_t ACQUISITION_PERIOD_MS = 1000;
const uint32_t CONTROL_PERIOD_MS     = 100;
const uint32_t DISPLAY_PERIOD_MS     = 500;

// Control constants
const float DEFAULT_SETPOINT = 20.0f;
const float SETPOINT_MIN     = 15.0f;
const float SETPOINT_MAX     = 60.0f;
const float SETPOINT_STEP    = 1.0f;

// Default PID gains (tunable via serial)
const float DEFAULT_KP = 5.0f;
const float DEFAULT_KI = 0.3f;
const float DEFAULT_KD = 1.0f;

// Hardware objects
LcdI2c      *lcd        = nullptr;
Dht11Sensor *dht11Sensor = nullptr;
Motor       *motor      = nullptr;
Joystick    *joystick   = nullptr;
Led         *led        = nullptr;

// Synchronization
kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semControlDisplay;

// Shared data – initial values
SharedData sharedData = {
    false, {0}, 0,      // serial interface
    20.0f,              // setpoint
    0.0f,               // temperature
    0.0f,               // pidOutput
    DEFAULT_KP, DEFAULT_KI, DEFAULT_KD,
    0.0f, 0.0f          // integral, prevError
};

}
