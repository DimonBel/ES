#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

void vTaskActuatorControl(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACTUATOR_CONTROL_PERIOD_MS);

    printf("[ACTUATOR_CTRL] Task started (period: %dms)\n", ACTUATOR_CONTROL_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (actuator == nullptr || signalConditioner == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        // Read button state
        bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
        uint32_t currentTime = millis();

        // Check for serial commands
        if (sharedData.serial_command_received) {
            sharedData.serial_command_received = false;
            sharedData.actuator_command_time = currentTime;

            const char* cmd = sharedData.serial_command_buffer;
            printf("[ACTUATOR_CTRL] Serial command: %s\n", cmd);

            if (strcmp(cmd, "on") == 0) {
                sharedData.actuator_command = true;
                printf("[ACTUATOR_CTRL] Command: ON\n");
            } else if (strcmp(cmd, "off") == 0) {
                sharedData.actuator_command = false;
                printf("[ACTUATOR_CTRL] Command: OFF\n");
            } else if (strcmp(cmd, "toggle") == 0) {
                sharedData.actuator_command = !sharedData.actuator_state;
                printf("[ACTUATOR_CTRL] Command: TOGGLE\n");
            } else if (strcmp(cmd, "status") == 0) {
                printf("[ACTUATOR_CTRL] Status: %s\n", actuator->getStateString());
                printf("[ACTUATOR_CTRL] Conditioned: %s\n", sharedData.actuator_conditioned ? "ON" : "OFF");
                printf("[ACTUATOR_CTRL] Toggle count: %lu\n", sharedData.actuator_toggle_count);
            }

            // Clear command buffer
            memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
            sharedData.serial_command_index = 0;
        }

        // Check button press for toggle
        if (buttonPressed && !sharedData.actuator_state) {
            // Button pressed while actuator is OFF - toggle to ON
            kernel_primitives::delayMs(50);  // Simple debounce
            if (digitalRead(BUTTON_PIN) == LOW) {
                sharedData.actuator_command = true;
                sharedData.actuator_command_time = currentTime;
                printf("[ACTUATOR_CTRL] Button press: ON\n");
            }
        } else if (!buttonPressed && sharedData.actuator_state) {
            // Button released while actuator is ON - toggle to OFF
            kernel_primitives::delayMs(50);  // Simple debounce
            if (digitalRead(BUTTON_PIN) == HIGH) {
                sharedData.actuator_command = false;
                sharedData.actuator_command_time = currentTime;
                printf("[ACTUATOR_CTRL] Button release: OFF\n");
            }
        }

        // Signal display task to update
        semActuatorDisplay.give();
    }
}

void vTaskSignalConditioning(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACTUATOR_CONTROL_PERIOD_MS);

    printf("[SIGNAL_COND] Task started (period: %dms)\n", ACTUATOR_CONTROL_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (signalConditioner == nullptr || actuator == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        // Apply signal conditioning
        bool conditionedSignal = signalConditioner->conditionSignal(
            sharedData.actuator_command,
            ACTUATOR_DEBOUNCE_TIME_MS,
            ACTUATOR_VALIDATION_TIME_MS
        );

        // Update shared data
        sharedData.actuator_conditioned = conditionedSignal;

        // Control actuator based on conditioned signal
        if (conditionedSignal != sharedData.actuator_state) {
            // State changed
            sharedData.actuator_state = conditionedSignal;
            sharedData.actuator_toggle_count++;

            if (conditionedSignal) {
                actuator->turnOn();
            } else {
                actuator->turnOff();
            }
        }
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISPLAY] Task started (period: %dms)\n", DISPLAY_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];

        // Wait for actuator display signal
        if (semActuatorDisplay.take(pdMS_TO_TICKS(100))) {
            // Display actuator state
            const char* stateStr = sharedData.actuator_state ? "ON" : "OFF";
            const char* cmdStr = sharedData.actuator_command ? "ON" : "OFF";

            snprintf(line1, sizeof(line1), "Actuator: %s", stateStr);
            snprintf(line2, sizeof(line2), "Cmd: %s Tog:%lu", cmdStr, sharedData.actuator_toggle_count);

            updateLCD(line1, line2);
        }
    }
}

bool createApplicationTasks() {
    bool actuatorCtrlCreated = kernel_primitives::createTask(
        vTaskActuatorControl,
        "ActuatorCtrl",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_ACTUATOR,
        nullptr
    );

    bool signalCondCreated = kernel_primitives::createTask(
        vTaskSignalConditioning,
        "SignalCond",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_CONDITIONING,
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

    return actuatorCtrlCreated && signalCondCreated && displayCreated;
}

}
