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
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (soundSensor == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        // Read analog value
        uint16_t currentValue = soundSensor->readAnalog();
        sharedData.analog_value = currentValue;

        // Read digital value for sound detection
        bool soundDetected = soundSensor->isSoundDetected();
        uint32_t currentTime = xTaskGetTickCount();

        // Threshold detection with hysteresis
        bool currentThresholdState = false;
        if (sharedData.led_state) {
            // LED is ON, require value below (threshold - hysteresis) to turn off
            currentThresholdState = (currentValue > (SOUND_THRESHOLD - SOUND_HYSTERESIS));
        } else {
            // LED is OFF, require value above threshold to turn on
            currentThresholdState = (currentValue > SOUND_THRESHOLD);
        }

        // Debounce: only change state if stable for minimum time
        if (currentThresholdState != sharedData.threshold_exceeded) {
            if (soundDetected || (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(SOUND_DEBOUNCE_TIME))) {
                sharedData.threshold_exceeded = currentThresholdState;
                
                if (sharedData.threshold_exceeded) {
                    sharedData.sound_count++;
                    sharedData.last_sound_time = currentTime;
                    sharedData.led_state = true;
                    sharedData.led_turn_off_time = currentTime + pdMS_TO_TICKS(1000);
                    sharedData.task_state = 1;

                    semSoundDisplay.give();
                    semSoundLED.give();

                    printf("[DETECT] Sound detected! Analog: %d, Threshold: %d\n", 
                           currentValue, SOUND_THRESHOLD);
                } else {
                    sharedData.led_state = false;
                    sharedData.task_state = 0;

                    semSoundDisplay.give();

                    printf("[DETECT] Sound level below threshold. Analog: %d\n", currentValue);
                }
            }
        }

        // Also check digital detection for immediate response
        if (soundDetected && !sharedData.sound_detected) {
            sharedData.sound_detected = true;
            sharedData.sound_count++;
            sharedData.last_sound_time = currentTime;
            
            // Trigger LED pulse for 1 second
            sharedData.led_state = true;
            sharedData.led_turn_off_time = currentTime + pdMS_TO_TICKS(1000);
            semSoundLED.give();
            
            printf("[DETECT] Digital sound detected! Count: %lu\n", sharedData.sound_count);
        } else if (!soundDetected) {
            sharedData.sound_detected = false;
        }
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];

        snprintf(line1, sizeof(line1), "Count:");
        snprintf(line2, sizeof(line2), "%lu", sharedData.sound_count);

        updateLCD(line1, line2);
    }
}

void vTaskLED(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        uint32_t currentTime = xTaskGetTickCount();
        
        // Check if LED should be turned off (1-second timeout)
        if (sharedData.led_state && currentTime >= sharedData.led_turn_off_time) {
            sharedData.led_state = false;
            if (led) {
                led->off();
                printf("[LED] LED OFF (timeout)\n");
            }
        }

        // Turn on LED when signal received
        if (semSoundLED.take(0)) {
            if (led && sharedData.led_state) {
                led->on();
                printf("[LED] LED ON (sound detected)\n");
            }
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
