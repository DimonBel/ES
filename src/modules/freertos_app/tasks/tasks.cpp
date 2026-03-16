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
    static bool wasAboveThreshold = false;  // Track if sound was previously above threshold

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (soundSensor == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        // Read analog value
        uint16_t currentValue = soundSensor->readAnalog();
        sharedData.analog_value = currentValue;

        uint32_t currentTime = xTaskGetTickCount();

        // Trigger only when sound goes from below to above threshold (rising edge)
        if (currentValue > SOUND_THRESHOLD && !wasAboveThreshold) {
            wasAboveThreshold = true;

            // Prevent too frequent triggers (minimum 5000ms between triggers)
            if (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(5000)) {
                sharedData.sound_count++;
                sharedData.last_sound_time = currentTime;
                sharedData.led_state = true;
                sharedData.led_turn_off_time = currentTime + pdMS_TO_TICKS(1000);

                semSoundDisplay.give();
                semSoundLED.give();

                printf("[DETECT] Sound detected! Analog: %d, Threshold: %d\n",
                       currentValue, SOUND_THRESHOLD);
            }
        }

        // Reset flag when sound goes below threshold
        if (currentValue <= SOUND_THRESHOLD) {
            wasAboveThreshold = false;
        }
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // Update every 100ms for faster response
    const TickType_t soundDisplayDuration = pdMS_TO_TICKS(2000);  // Show sound for 2 seconds

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];
        TickType_t currentTime = xTaskGetTickCount();
        bool showSound = false;

        // Check if we should display sound (if clap was detected recently)
        if ((currentTime - sharedData.last_sound_time) < soundDisplayDuration) {
            showSound = true;
            // Display sound level without percentages
            snprintf(line1, sizeof(line1), "Sound --> %d", sharedData.analog_value);
            snprintf(line2, sizeof(line2), "Clap!");
            printf("[DISPLAY] Showing sound: %s / %s\n", line1, line2);
        } else {
            // Display temperature
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
    for (;;) {
        uint32_t currentTime = xTaskGetTickCount();

        // Check if RGB LED should be turned back to red (1-second timeout after clap)
        if (sharedData.led_state && currentTime >= sharedData.led_turn_off_time) {
            sharedData.led_state = false;
            if (rgbLed) {
                rgbLed->red();
                printf("[LED] RGB LED RED (idle)\n");
            }
        }

        // Turn RGB LED green when clap signal received
        if (semSoundLED.take(0)) {
            if (rgbLed && sharedData.led_state) {
                rgbLed->green();
                printf("[LED] RGB LED GREEN (clap detected)\n");
            }
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
