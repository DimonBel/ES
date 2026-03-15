#ifndef FREERTOS_APP_STATE_H
#define FREERTOS_APP_STATE_H

#include <Arduino.h>
#include "led/led.h"
#include "lcd/lcd.h"
#include "sound_sensor/sound_sensor.h"
#include "ds18b20/ds18b20.h"
#include "rgb_led/rgb_led.h"
#include "kernel_primitives/mutex/mutex.h"
#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace freertos_app::internal {

struct SharedData {
    // Sound sensor data
    uint16_t analog_value;
    bool sound_detected;
    bool threshold_exceeded;
    uint32_t sound_count;
    uint8_t task_state;
    uint32_t last_sound_time;
    bool led_state;
    uint32_t led_turn_off_time;
    
    // Temperature sensor data
    float temperature;
    float temperature_filtered;
    bool temperature_available;
    uint32_t last_temperature_time;
    
    // Filter buffer for median filter
    float temperature_buffer[5];
    uint8_t temperature_buffer_index;
};

extern const uint8_t SOUND_SENSOR_D0_PIN;
extern const uint8_t SOUND_SENSOR_A0_PIN;
extern const uint8_t LED_PIN;
extern const uint8_t LCD_SDA_PIN;
extern const uint8_t LCD_SCL_PIN;

// Temperature sensor pins
extern const uint8_t DS18B20_PIN;

// RGB LED pins
extern const uint8_t RGB_LED_R_PIN;
extern const uint8_t RGB_LED_G_PIN;
extern const uint8_t RGB_LED_B_PIN;

extern const uint32_t TASK_STACK_SIZE;
extern const UBaseType_t TASK_PRIORITY_DETECT;
extern const UBaseType_t TASK_PRIORITY_DISPLAY;
extern const UBaseType_t TASK_PRIORITY_LED;
extern const UBaseType_t TASK_PRIORITY_TEMP;

extern const uint16_t SOUND_THRESHOLD;
extern const uint16_t SOUND_HYSTERESIS;
extern const uint32_t SOUND_DEBOUNCE_TIME;

extern const float TEMPERATURE_THRESHOLD_HIGH;
extern const float TEMPERATURE_THRESHOLD_LOW;

extern Led *led;
extern LcdI2c *lcd;
extern SoundSensor *soundSensor;
extern DS18B20 *tempSensor;
extern RgbLed *rgbLed;

extern kernel_primitives::Mutex lcdMutex;
extern kernel_primitives::BinarySemaphore semSoundDisplay;
extern kernel_primitives::BinarySemaphore semSoundLED;
extern kernel_primitives::BinarySemaphore semTempDisplay;

extern SharedData sharedData;

}

#endif