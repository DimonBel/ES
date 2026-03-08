#include "freertos_app/state/state.h"

namespace freertos_app::internal {

const uint8_t SOUND_SENSOR_D0_PIN = 12;
const uint8_t SOUND_SENSOR_A0_PIN = 32;
const uint8_t LED_PIN = 14;
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DETECT = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_LED = tskIDLE_PRIORITY + 1;

const uint16_t SOUND_THRESHOLD = 2000;
const uint16_t SOUND_HYSTERESIS = 100;
const uint32_t SOUND_DEBOUNCE_TIME = 50;

Led *led = nullptr;
LcdI2c *lcd = nullptr;
SoundSensor *soundSensor = nullptr;

kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semSoundDisplay;
kernel_primitives::BinarySemaphore semSoundLED;

SharedData sharedData = {0, false, false, 0, 0, 0, false, 0};

}
