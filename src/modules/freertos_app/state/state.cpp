#include "freertos_app/state/state.h"

namespace freertos_app::internal {

// LCD
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// L298N motor driver
const uint8_t MOTOR_IN1_PIN = 25;
const uint8_t MOTOR_IN2_PIN = 26;
const uint8_t MOTOR_ENA_PIN = 27;

// Sensors
const uint8_t POTENTIOMETER_PIN = 34;   // SetPoint
const uint8_t JOYSTICK_X_PIN    = 35;   // Value (position sensor)
const uint8_t JOYSTICK_Y_PIN    = 32;
const uint8_t JOYSTICK_SW_PIN   = 0;
const uint8_t LED_PIN           = 2;    // Built-in LED (free from L298N conflict)

// Task configuration
const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DISPLAY     = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_ACQUISITION = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_CONTROL     = tskIDLE_PRIORITY + 3;

// Task periods
const uint32_t ACQUISITION_PERIOD_MS = 50;
const uint32_t CONTROL_PERIOD_MS     = 50;
const uint32_t DISPLAY_PERIOD_MS     = 200;

// Control constants
const int MOTOR_SATURATION_SPEED = 50;   // 50 % fixed power (saturation)
const int DEFAULT_HYSTERESIS     = 5;    // 5 % deadband

// Hardware objects
LcdI2c       *lcd         = nullptr;
Motor        *motor       = nullptr;
Joystick     *joystick    = nullptr;
Led          *led         = nullptr;
Potentiometer *potentiometer = nullptr;

// Synchronization
kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semControlDisplay;

// Shared data – initial values
SharedData sharedData = {
    false, {0}, 0,   // serial interface
    50, 50, 0,       // setpoint, value, output
    5,               // hysteresis
    50               // motorSpeed
};

}
