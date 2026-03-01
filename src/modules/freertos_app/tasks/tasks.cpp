#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

void vTaskDetect(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        kernel_primitives::delayUntilMs(&xLastWakeTime, 20);

        if (joystick == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        joystick->scan();

        if (joystick->isPressed()) {
            if (!sharedData.button_pressed) {
                sharedData.button_pressed = true;
                sharedData.task_state = 1;

                semPressDisplay.give();
                semPressLED.give();

                printf("[DETECT] Button pressed\n");
            }
        } else {
            if (sharedData.button_pressed) {
                uint32_t duration = joystick->getPressDuration();
                sharedData.button_pressed = false;
                sharedData.press_duration = duration;
                sharedData.new_press_detected = true;
                sharedData.task_state = 2;

                semReleaseDisplay.give();
                semReleaseLED.give();

                printf("[DETECT] Button released! Duration: %lu ms\n", duration);
            }
        }
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t resultDeadline = 0;
    bool resultVisible = false;

    for (;;) {
        if (semPressDisplay.take(100)) {
            updateLCD("Pressing...", "Button");
            resultVisible = false;
        }

        if (semReleaseDisplay.take(100)) {
            char line1[16];
            char line2[16];

            if (sharedData.press_duration > 500) {
                snprintf(line1, sizeof(line1), "Green LED:");
            } else {
                snprintf(line1, sizeof(line1), "Red LED:");
            }
            snprintf(line2, sizeof(line2), "%lu ms", sharedData.press_duration);

            updateLCD(line1, line2);
            resultDeadline = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
            resultVisible = true;
        }

        if (resultVisible && xTaskGetTickCount() >= resultDeadline) {
            updateLCD("Press Joystick", "Button");
            sharedData.task_state = 0;
            sharedData.new_press_detected = false;
            resultVisible = false;
        }

        kernel_primitives::delayMs(50);
    }
}

void vTaskLED(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        if (semPressLED.take(100)) {
            if (ledR) ledR->off();
            if (ledG) ledG->off();
            if (ledY) ledY->on();
            printf("[LED] Yellow ON, others OFF\n");
        }

        if (semReleaseLED.take(100)) {
            if (ledY) ledY->off();
            if (ledR) ledR->off();
            if (ledG) ledG->off();

            if (sharedData.press_duration > 500) {
                if (ledG) ledG->on();
                printf("[LED] Green ON (duration > 500ms)\n");
            } else {
                if (ledR) ledR->on();
                printf("[LED] Red ON (duration <= 500ms)\n");
            }
        }

        if (sharedData.task_state == 0) {
            if (ledR) ledR->off();
            if (ledG) ledG->off();
            if (ledY) ledY->off();
        }

        kernel_primitives::delayMs(50);
    }
}

bool createApplicationTasks() {
    bool detectCreated = kernel_primitives::createTask(
        vTaskDetect,
        "Detect",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_DETECT,
        nullptr
    );

    bool displayCreated = kernel_primitives::createTask(
        vTaskDisplay,
        "Display",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_DISPLAY,
        nullptr
    );

    bool ledCreated = kernel_primitives::createTask(
        vTaskLED,
        "LED",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_LED,
        nullptr
    );

    return detectCreated && displayCreated && ledCreated;
}

}
