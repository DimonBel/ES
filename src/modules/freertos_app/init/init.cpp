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

    printf("\n=== LAB 2.2 - Sound Detection System ===\n");
    printf("Sound Sensor: D0=%d, A0=%d\n", SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    printf("LED: %d\n", LED_PIN);
    printf("LCD: I2C SDA=%d, SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("==========================================\n");

    led = new Led(LED_PIN);
    led->begin();
    led->off();
    printf("LED initialized\n");

    soundSensor = new SoundSensor(SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    soundSensor->begin();
    soundSensor->setThreshold(SOUND_THRESHOLD);
    soundSensor->setHysteresis(SOUND_HYSTERESIS);
    printf("Sound sensor initialized\n");
    printf("  Threshold: %d\n", SOUND_THRESHOLD);
    printf("  Hysteresis: %d\n", SOUND_HYSTERESIS);

    printf("Testing LED...\n");
    if (led) { led->on(); kernel_primitives::delayMs(200); led->off(); }
    printf("LED test complete\n");

    printf("Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Sound Detection");
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
        printf("=====================================\n");
        printf("Sound detection active...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }
}

}
