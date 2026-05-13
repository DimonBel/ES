#include "freertos_app/state/state.h"

namespace freertos_app::internal {

// LCD
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Button / LED
const uint8_t JOYSTICK_SW_PIN = 17;
const uint8_t LED_PIN         = 4;

// Task configuration
const uint32_t    TASK_STACK_SIZE        = 4096;
const UBaseType_t TASK_PRIORITY_FSM     = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;

// Task periods
const uint32_t FSM_PERIOD_MS     = 100;
const uint32_t DISPLAY_PERIOD_MS = 500;
const uint32_t DEBOUNCE_MS       = 200;

// Hardware objects
LcdI2c   *lcd      = nullptr;
Joystick *joystick = nullptr;
Led      *led      = nullptr;

// Synchronization
kernel_primitives::Mutex lcdMutex;

// Shared data
SharedData sharedData = {
    LedState::OFF,  // ledState
    0               // lastPressTime
};

}
