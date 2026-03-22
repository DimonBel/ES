#include "freertos_app/state/state.h"

namespace freertos_app::internal {

// LCD pins
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Actuator and Button pins (Lab 4.1)
const uint8_t ACTUATOR_PIN = 23;
const uint8_t BUTTON_PIN = 18;

// Additional UI pins
const uint8_t JOYSTICK_X_PIN = 34;
const uint8_t JOYSTICK_Y_PIN = 35;
const uint8_t JOYSTICK_SW_PIN = 25;
const uint8_t LED_PIN = 26;

const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_ACTUATOR = tskIDLE_PRIORITY + 3;    // Lab 4.1
const UBaseType_t TASK_PRIORITY_CONDITIONING = tskIDLE_PRIORITY + 3; // Lab 4.1

// Actuator control configuration (Lab 4.1)
const uint32_t ACTUATOR_CONTROL_PERIOD_MS = 50;
const uint32_t ACTUATOR_DEBOUNCE_TIME_MS = 50;
const uint32_t ACTUATOR_VALIDATION_TIME_MS = 100;
const uint32_t DISPLAY_PERIOD_MS = 500;

LcdI2c *lcd = nullptr;
Actuator *actuator = nullptr;          // Lab 4.1
SignalConditioner *signalConditioner = nullptr; // Lab 4.1
Joystick *joystick = nullptr;
Led *led = nullptr;

kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semActuatorDisplay; // Lab 4.1

SharedData sharedData = {
    false, false, false, 0, 0, false, {0}, 0
};

}
