#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "led/led.h"
#include "lcd/lcd.h"
#include "joystick/joystick.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace freertos_app::internal {

struct SharedData {
    uint32_t press_duration;
    bool new_press_detected;
    bool button_pressed;
    uint8_t task_state;
};

extern const uint8_t JOYSTICK_X_PIN;
extern const uint8_t JOYSTICK_Y_PIN;
extern const uint8_t JOYSTICK_SW_PIN;
extern const uint8_t LED_RED_PIN;
extern const uint8_t LED_GREEN_PIN;
extern const uint8_t LED_YELLOW_PIN;

extern const uint32_t TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DETECT;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_LED;

extern Led *ledG;
extern Led *ledR;
extern Led *ledY;
extern LcdI2c *lcd;
extern Joystick *joystick;

extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semPressDisplay;
extern kernel_primitives::BinarySemaphore semReleaseDisplay;
extern kernel_primitives::BinarySemaphore semPressLED;
extern kernel_primitives::BinarySemaphore semReleaseLED;

extern SharedData sharedData;

}

#endif