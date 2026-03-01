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

    printf("\n=== LAB 3.2.2 - FreeRTOS JOYSTICK & LED ===\n");
    printf("Joystick: X=A0, Y=A1, SW=D3\n");
    printf("LED R: %d, LED G: %d, LED Y: %d\n", LED_RED_PIN, LED_GREEN_PIN, LED_YELLOW_PIN);
    printf("LCD: I2C (0x27)\n");
    printf("==========================================\n");

    ledG = new Led(LED_GREEN_PIN);
    ledR = new Led(LED_RED_PIN);
    ledY = new Led(LED_YELLOW_PIN);
    ledG->begin();
    ledR->begin();
    ledY->begin();
    ledG->off();
    ledR->off();
    ledY->off();
    printf("LEDs initialized\n");

    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("Joystick initialized\n");

    printf("Testing LEDs...\n");
    if (ledG) { ledG->on(); kernel_primitives::delayMs(100); ledG->off(); }
    if (ledR) { ledR->on(); kernel_primitives::delayMs(100); ledR->off(); }
    if (ledY) { ledY->on(); kernel_primitives::delayMs(100); ledY->off(); }
    printf("LED test complete\n");

    printf("Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Press Joystick");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("Button");
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
        printf("Press joystick button...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }
}

}
