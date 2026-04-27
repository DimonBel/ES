#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

// ─── Task 1: Acquisition (50 ms) ─────────────────────────────────────────────
// Reads SetPoint (potentiometer) and Value (joystick X = position sensor).
void vTaskAcquisition(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACQUISITION_PERIOD_MS);

    printf("[ACQ] Task started (%dms)\n", ACQUISITION_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // SetPoint: potentiometer → 0-100 %
        if (potentiometer != nullptr) {
            potentiometer->scan();
            sharedData.setpoint = potentiometer->getPercentage();
        }

        // Value: joystick X → ESP32 ADC is 12-bit (0-4095) → 0-100 %
        if (joystick != nullptr) {
            joystick->scan();
            uint16_t rawX = joystick->getX();
            sharedData.value = (int)((rawX * 100UL) / 4095);
        }
    }
}

// ─── Task 2: ON-OFF Control with Hysteresis (50 ms) ──────────────────────────
// error = SetPoint – Value
//  error >  +hysteresis  → FORWARD  (motor at 50 % saturation)
//  error <  -hysteresis  → BACKWARD (motor at 50 % saturation)
//  |error| ≤  hysteresis → maintain last state (deadband / hysteresis)
void vTaskOnOffControl(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_PERIOD_MS);

    printf("[CTRL] Task started (%dms)\n", CONTROL_PERIOD_MS);

    int  lastOutput   = 0;
    bool emergencyStop = false;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (motor == nullptr) continue;

        // ── Serial command handling ────────────────────────────────────────
        if (sharedData.serial_command_received) {
            sharedData.serial_command_received = false;
            const char *cmd = sharedData.serial_command_buffer;

            if (strcmp(cmd, "stop") == 0) {
                emergencyStop = true;
                motor->stop();
                lastOutput = 0;
                sharedData.output = 0;
                if (led != nullptr) led->off();
                printf("[CTRL] Emergency STOP\n");

            } else if (strcmp(cmd, "run") == 0) {
                emergencyStop = false;
                printf("[CTRL] Control re-enabled\n");

            } else if (strcmp(cmd, "status") == 0) {
                printf("[STATUS] SP:%d%% Val:%d%% Dir:%s H:%d%% Speed:%d%%\n",
                    sharedData.setpoint, sharedData.value,
                    motor->getDirectionString(),
                    sharedData.hysteresis, sharedData.motorSpeed);

            } else if (strncmp(cmd, "hyst", 4) == 0 && cmd[4] != '\0') {
                // "hyst8" → hysteresis = 8 %
                int h = atoi(cmd + 4);
                if (h >= 1 && h <= 20) {
                    sharedData.hysteresis = h;
                    printf("[CTRL] Hysteresis set to %d%%\n", h);
                } else {
                    printf("[CTRL] Invalid hysteresis (1-20): %d\n", h);
                }
            }

            memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
            sharedData.serial_command_index = 0;
        }

        if (emergencyStop) continue;

        // ── ON-OFF controller with hysteresis ─────────────────────────────
        int sp    = sharedData.setpoint;
        int val   = sharedData.value;
        int hyst  = sharedData.hysteresis;
        int speed = sharedData.motorSpeed;
        int error = sp - val;

        int newOutput = lastOutput;   // stay in current state inside deadband

        if (error > hyst) {
            newOutput = 1;    // FORWARD – value too low, drive up
        } else if (error < -hyst) {
            newOutput = -1;   // BACKWARD – value too high, drive down
        }

        // Apply only on state change (no redundant writes)
        if (newOutput != lastOutput) {
            lastOutput = newOutput;

            if (newOutput == 1) {
                motor->forward(speed);
                if (led != nullptr) led->on();
                printf("[CTRL] FORWARD  SP:%d Val:%d Err:%+d\n", sp, val, error);
            } else if (newOutput == -1) {
                motor->backward(speed);
                if (led != nullptr) led->on();
                printf("[CTRL] BACKWARD SP:%d Val:%d Err:%+d\n", sp, val, error);
            } else {
                motor->stop();
                if (led != nullptr) led->off();
                printf("[CTRL] STOP     SP:%d Val:%d Err:%+d\n", sp, val, error);
            }
        }

        sharedData.output = newOutput;

        // Signal display task
        semControlDisplay.give();
    }
}

// ─── Task 3: Display (200 ms) ─────────────────────────────────────────────────
// LCD: line 1 → "SP:xxx% V:xxx%"   line 2 → "Dir:XXX  H:xx%"
// Serial Plotter: "SetPoint:xx Value:xx Output:xx\n"
//   Output is scaled to –100 / 0 / +100 for easy plotting.
void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISP] Task started (%dms)\n", DISPLAY_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        int sp  = sharedData.setpoint;
        int val = sharedData.value;
        int out = sharedData.output;

        // ── LCD update ────────────────────────────────────────────────────
        if (semControlDisplay.take(pdMS_TO_TICKS(150))) {
            const char *dirStr = (motor != nullptr) ? motor->getDirectionString() : "---";
            char line1[17], line2[17];

            // "SP: 50% V: 30%  " – max 16 chars
            snprintf(line1, sizeof(line1), "SP:%3d%% V:%3d%%", sp, val);
            // "Dir:FWD  H: 5%  "
            snprintf(line2, sizeof(line2), "Dir:%-3s  H:%2d%%", dirStr, sharedData.hysteresis);

            updateLCD(line1, line2);
        }

        // ── Arduino Serial Plotter ────────────────────────────────────────
        // Label:value pairs separated by spaces.
        // Output: +100 = FORWARD, 0 = STOP, –100 = BACKWARD
        int plotOut = out * 100;
        printf("SetPoint:%d Value:%d Output:%d\n", sp, val, plotOut);
    }
}

// ─── Task creation ────────────────────────────────────────────────────────────
bool createApplicationTasks() {
    bool ok = true;

    ok &= kernel_primitives::createTask(
        vTaskAcquisition, "Acquisition",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_ACQUISITION, nullptr);

    ok &= kernel_primitives::createTask(
        vTaskOnOffControl, "OnOffCtrl",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_CONTROL, nullptr);

    ok &= kernel_primitives::createTask(
        vTaskDisplay, "Display",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_DISPLAY, nullptr);

    return ok;
}

}
