#include "freertos_app/init/init.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "freertos_app/tasks/tasks.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

void setupApplication() {
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    printf("\n=== LAB 3.2 - Dual Sensor Monitoring ===\n");
    printf("Sound Sensor: D0=%d, A0=%d\n", SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    printf("Temp Sensor: DS18B20 on pin %d\n", DS18B20_PIN);
    printf("LED: %d\n", LED_PIN);
    printf("RGB LED: R=%d, G=%d, B=%d\n", RGB_LED_R_PIN, RGB_LED_G_PIN, RGB_LED_B_PIN);
    printf("LCD: I2C SDA=%d, SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("==========================================\n");

    // Initialize RGB LED and set it to red
    rgbLed = new RgbLed(RGB_LED_R_PIN, RGB_LED_G_PIN, RGB_LED_B_PIN);
    rgbLed->begin();
    rgbLed->red();
    printf("RGB LED initialized (RED)\n");

    soundSensor = new SoundSensor(SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    soundSensor->begin();
    soundSensor->setThreshold(SOUND_THRESHOLD);
    soundSensor->setHysteresis(SOUND_HYSTERESIS);
    printf("Sound sensor initialized\n");
    printf("  Threshold: %d\n", SOUND_THRESHOLD);
    printf("  Hysteresis: %d\n", SOUND_HYSTERESIS);

    // Initialize DS18B20 temperature sensor
    tempSensor = new DS18B20(DS18B20_PIN);
    tempSensor->begin();
    tempSensor->setResolution(12);  // 12-bit resolution (0.0625°C precision)
    printf("DS18B20 temperature sensor initialized\n");
    printf("  Resolution: 12 bits\n");

    printf("Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Dual Sensor");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("System Ready");
        kernel_primitives::delayMs(100);
        printf("LCD initialized\n");
    } else {
        printf("ERROR: Failed to create LCD object!\n");
    }

    if (!initSyncPrimitives()) {
        printf("ERROR: Failed to create semaphores/mutex!\n");
        while (1);
    }

    printf("Creating FreeRTOS tasks...\n");

    if (createApplicationTasks()) {
        printf("=== FREE-RTOS SCHEDULER STARTED ===\n");
        printf("Tasks running:\n");
        printf("  - Detect (priority %d)\n", TASK_PRIORITY_DETECT);
        printf("  - Display (priority %d)\n", TASK_PRIORITY_DISPLAY);
        printf("  - LED (priority %d)\n", TASK_PRIORITY_LED);
        printf("  - Temperature (priority %d)\n", TASK_PRIORITY_TEMP);
        printf("=====================================\n");
        printf("Dual sensor monitoring active...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }
}

}
