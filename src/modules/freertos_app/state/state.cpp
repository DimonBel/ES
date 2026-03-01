#include "freertos_app/state/state.h"

namespace freertos_app::internal {

const uint8_t JOYSTICK_X_PIN = 34;
const uint8_t JOYSTICK_Y_PIN = 35;
const uint8_t JOYSTICK_SW_PIN = 2;
const uint8_t LED_RED_PIN = 12;
const uint8_t LED_GREEN_PIN = 14;
const uint8_t LED_YELLOW_PIN = 13;

const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DETECT = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_LED = tskIDLE_PRIORITY + 1;

Led *ledG = nullptr;
Led *ledR = nullptr;
Led *ledY = nullptr;
LcdI2c *lcd = nullptr;
Joystick *joystick = nullptr;

kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semPressDisplay;
kernel_primitives::BinarySemaphore semReleaseDisplay;
kernel_primitives::BinarySemaphore semPressLED;
kernel_primitives::BinarySemaphore semReleaseLED;

SharedData sharedData = {0, false, false, 0};

}
