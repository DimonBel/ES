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
    static bool wasAboveThreshold = false;
    static bool baselineInitialized = false;
    static int32_t baseline = 0;
    static TickType_t lastSoundActiveTime = 0;

    const int32_t deviationThreshold = SOUND_THRESHOLD;
    const TickType_t soundHoldTime = pdMS_TO_TICKS(250);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (soundSensor == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        // Read conditioned analog value (saturation + median + weighted average)
        uint16_t currentValue = soundSensor->readConditionedAnalog();
        uint16_t rawValue = soundSensor->getRawAnalogValue();
        sharedData.analog_value = currentValue;

        if (!baselineInitialized) {
            baseline = static_cast<int32_t>(currentValue);
            baselineInitialized = true;
        }

        const int32_t sample = static_cast<int32_t>(currentValue);
        const int32_t deviation = sample - baseline;
        const int32_t absDeviation = deviation < 0 ? -deviation : deviation;

        const TickType_t currentTime = xTaskGetTickCount();

        const bool instantSound = absDeviation >= deviationThreshold;
        if (instantSound) {
            lastSoundActiveTime = currentTime;
        }

        const bool soundActive = (currentTime - lastSoundActiveTime) < soundHoldTime;

        // Real-time sound state used by display and LED tasks.
        sharedData.sound_detected = soundActive;
        sharedData.threshold_exceeded = soundActive;
        sharedData.led_state = soundActive;

        // Adapt baseline slowly to ambient noise when no sound is active.
        if (!soundActive) {
            baseline = (baseline * 31 + sample) / 32;
        }

        // Count events on rising edge with short debounce.
        if (soundActive && !wasAboveThreshold &&
            (currentTime - sharedData.last_sound_time) >= pdMS_TO_TICKS(SOUND_DEBOUNCE_TIME)) {
            sharedData.sound_count++;
            sharedData.last_sound_time = currentTime;
                 printf("[DETECT] Sound active. Raw: %d, Filtered: %d, Baseline: %ld, Delta: %ld, Threshold: %ld\n",
                     rawValue,
                   currentValue,
                   static_cast<long>(baseline),
                   static_cast<long>(absDeviation),
                   static_cast<long>(deviationThreshold));
        }

        wasAboveThreshold = soundActive;
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];

        // Show sound data immediately while sound is active.
        if (sharedData.sound_detected) {
            snprintf(line1, sizeof(line1), "Sound --> %d", sharedData.analog_value);
            snprintf(line2, sizeof(line2), "Active");
            printf("[DISPLAY] Showing sound: %s / %s\n", line1, line2);
        } else {
            if (sharedData.temperature_available) {
                snprintf(line1, sizeof(line1), "Temp: %.1f C", sharedData.temperature);
                snprintf(line2, sizeof(line2), "Filt: %.1f C", sharedData.temperature_filtered);
                printf("[DISPLAY] Showing temp: %s / %s\n", line1, line2);
            } else {
                snprintf(line1, sizeof(line1), "Temp: ---.- C");
                snprintf(line2, sizeof(line2), "Waiting...");
                printf("[DISPLAY] Waiting: %s / %s\n", line1, line2);
            }
        }

        updateLCD(line1, line2);
    }
}

void vTaskLED(void *pvParameters) {
    (void)pvParameters;
    bool wasSoundActive = false;

    for (;;) {
        const bool soundActive = sharedData.led_state;

        if (rgbLed && (soundActive != wasSoundActive)) {
            if (soundActive) {
                rgbLed->green();
                printf("[LED] RGB LED GREEN (sound active)\n");
            } else {
                rgbLed->red();
                printf("[LED] RGB LED RED (idle)\n");
            }
            wasSoundActive = soundActive;
        }

        kernel_primitives::delayMs(50);
    }
}

void vTaskTemperature(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // 100ms period
    static bool conversionRequested = false;
    static uint8_t waitCycles = 0;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (tempSensor == nullptr) {
            continue;
        }

        if (!conversionRequested) {
            // Request temperature conversion
            if (tempSensor->requestTemperature()) {
                conversionRequested = true;
                waitCycles = 0;
            }
        } else {
            // Wait for conversion to complete (DS18B20 takes ~750ms for 12-bit resolution)
            waitCycles++;
            if (waitCycles >= 8) {  // 8 * 100ms = 800ms (enough for conversion)
                // Read temperature
                float temp = tempSensor->getTemperature();
                
                if (temp > -100.0f) {  // Valid temperature
                    sharedData.temperature = temp;
                    sharedData.temperature_available = true;
                    sharedData.last_temperature_time = xTaskGetTickCount();
                    
                    // Add to filter buffer (circular buffer)
                    sharedData.temperature_buffer[sharedData.temperature_buffer_index] = temp;
                    sharedData.temperature_buffer_index = (sharedData.temperature_buffer_index + 1) % 5;
                    
                    // Calculate median filter
                    float sorted[5];
                    for (int i = 0; i < 5; i++) {
                        sorted[i] = sharedData.temperature_buffer[i];
                    }
                    
                    // Simple bubble sort
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4 - i; j++) {
                            if (sorted[j] > sorted[j + 1]) {
                                float temp = sorted[j];
                                sorted[j] = sorted[j + 1];
                                sorted[j + 1] = temp;
                            }
                        }
                    }
                    
                    // Median is the middle value
                    sharedData.temperature_filtered = sorted[2];
                    
                    // Signal display task to update
                    semTempDisplay.give();
                    
                    printf("[TEMP] Temperature: %.2f°C, Filtered: %.2f°C\n", 
                           sharedData.temperature, sharedData.temperature_filtered);
                } else {
                    printf("[TEMP] Failed to read temperature\n");
                }
                
                conversionRequested = false;  // Ready for next conversion
            }
        }
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

    bool tempCreated = kernel_primitives::createTask(
        vTaskTemperature,
        "Temperature",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_TEMP,
        nullptr
    );

    return detectCreated && displayCreated && ledCreated && tempCreated;
}

}
