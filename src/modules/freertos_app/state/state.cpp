#include "freertos_app/state/state.h"

namespace freertos_app::internal {

// LCD pins
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Actuator and Button pins (Lab 4.1 - Binary)
const uint8_t ACTUATOR_PIN = 23;    // Relay control
const uint8_t BUTTON_PIN = 19;      // Manual toggle (changed from 18 to avoid servo conflict)

// Servo and Potentiometer pins (Lab 4.2 - Analog)
const uint8_t SERVO_PIN = 18;       // Servo PWM signal (user hardware setup)
const uint8_t POTENTIOMETER_PIN = 34; // Analog input for speed control (user hardware setup)

// Additional UI pins (reconfigured to avoid conflicts)
const uint8_t JOYSTICK_X_PIN = 35;  // Changed from 34 to avoid potentiometer conflict
const uint8_t JOYSTICK_Y_PIN = 32;  // Changed from 35 to avoid conflict
const uint8_t JOYSTICK_SW_PIN = 0;  // Changed from 18 to avoid servo conflict (GPIO 0 = BOOT button)
const uint8_t LED_PIN = 26;

const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_ACTUATOR = tskIDLE_PRIORITY + 3;      // Lab 4.1
const UBaseType_t TASK_PRIORITY_CONDITIONING = tskIDLE_PRIORITY + 3; // Lab 4.1
const UBaseType_t TASK_PRIORITY_SERVO = tskIDLE_PRIORITY + 3;        // Lab 4.2

// Actuator control configuration (Lab 4.1)
const uint32_t ACTUATOR_CONTROL_PERIOD_MS = 50;
const uint32_t ACTUATOR_DEBOUNCE_TIME_MS = 50;
const uint32_t ACTUATOR_VALIDATION_TIME_MS = 100;
const uint32_t DISPLAY_PERIOD_MS = 500;

// Servo control configuration (Lab 4.2)
const uint32_t SERVO_CONTROL_PERIOD_MS = 50;   // Servo update every 50ms
const uint32_t SERVO_COOLDOWN_MS = 250;        // Cooldown between speed changes

LcdI2c *lcd = nullptr;
Actuator *actuator = nullptr;          // Lab 4.1 - Binary actuator
SignalConditioner *signalConditioner = nullptr; // Lab 4.1
Joystick *joystick = nullptr;
Led *led = nullptr;
Servo *servo = nullptr;                // Lab 4.2 - Analog actuator
Potentiometer *potentiometer = nullptr; // Lab 4.2 - Analog input

kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semActuatorDisplay; // Lab 4.1

SharedData sharedData = {
    // Binary actuator data
    false, false, false, 0, 0, false, {0}, 0,
    // Servo data
    0, 0, 0, 0, true, 0
};

}
