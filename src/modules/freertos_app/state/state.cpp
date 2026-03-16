#include "freertos_app/state/state.h"

namespace freertos_app::internal {

const uint8_t SOUND_SENSOR_D0_PIN = 12;
const uint8_t SOUND_SENSOR_A0_PIN = 34;
const uint8_t LED_PIN = 14;
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Temperature sensor pin
const uint8_t DS18B20_PIN = 4;

// RGB LED pins
const uint8_t RGB_LED_R_PIN = 25;
const uint8_t RGB_LED_G_PIN = 26;
const uint8_t RGB_LED_B_PIN = 27;

const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DETECT = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_LED = tskIDLE_PRIORITY + 1;
const UBaseType_t TASK_PRIORITY_TEMP = tskIDLE_PRIORITY + 3;

const uint16_t SOUND_THRESHOLD = 120;
const uint16_t SOUND_HYSTERESIS = 50;
const uint32_t SOUND_DEBOUNCE_TIME = 50;

const float TEMPERATURE_THRESHOLD_HIGH = 30.0f;
const float TEMPERATURE_THRESHOLD_LOW = 20.0f;

Led *led = nullptr;
LcdI2c *lcd = nullptr;
SoundSensor *soundSensor = nullptr;
DS18B20 *tempSensor = nullptr;
RgbLed *rgbLed = nullptr;

kernel_primitives::Mutex lcdMutex;
kernel_primitives::BinarySemaphore semSoundDisplay;
kernel_primitives::BinarySemaphore semSoundLED;
kernel_primitives::BinarySemaphore semTempDisplay;

SharedData sharedData = {
    0, false, false, 0, 0, 0, false, 0,
    0.0f, 0.0f, false, 0,
    {20.0f, 20.0f, 20.0f, 20.0f, 20.0f}, 0
};

}
