#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

// Button: short press (<500 ms) = SP+1°C, long press (≥500 ms) = SP-1°C.
void vTaskAcquisition(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACQUISITION_PERIOD_MS);

    printf("[ACQ] Task started (%dms)\n", ACQUISITION_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Button is scanned in vTaskOnOffControl (100 ms) for better responsiveness

        // ── Temperature: read DHT11 or simulate if sensor absent ─────────
        if (dht11Sensor != nullptr) {
            float t = dht11Sensor->readTemperature();
            if (t > -100.0f) {
                sharedData.temperature = t;
            } else {
                // No real sensor: simulate heating/cooling for demo
                float &temp = sharedData.temperature;
                if (sharedData.relayOn) {
                    temp += 0.2f;
                } else {
                    temp -= 0.1f;
                }
                if (temp < 15.0f) temp = 15.0f;
                if (temp > 70.0f) temp = 70.0f;
                printf("[ACQ] SIMULATED Temp=%.1f C\n", temp);
            }
        }
    }
}

// Relay ON  when temp < setpoint - hysteresis  (below lower bound → heat)
// Relay OFF when temp > setpoint + hysteresis  (above upper bound → cool)
// Within deadband: maintain current relay state (no switching).
void vTaskOnOffControl(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_PERIOD_MS);

    printf("[CTRL] Task started (%dms)\n", CONTROL_PERIOD_MS);

    bool emergencyStop = false;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // ── Serial command handling ───────────
        if (sharedData.serial_command_received) {
            sharedData.serial_command_received = false;
            const char *cmd = sharedData.serial_command_buffer;

            if (strcmp(cmd, "stop") == 0) {
                emergencyStop = true;
                if (relay != nullptr) relay->turnOff();
                sharedData.relayOn = false;
                if (led != nullptr) led->off();
                printf("[CTRL] Emergency STOP – relay OFF\n");

            } else if (strcmp(cmd, "run") == 0) {
                emergencyStop = false;
                printf("[CTRL] Control re-enabled\n");

            } else if (strcmp(cmd, "status") == 0) {
                printf("[STATUS] SP:%.1f T:%.1f Relay:%s H:%.1f\n",
                       sharedData.setpoint, sharedData.temperature,
                       sharedData.relayOn ? "ON" : "OFF",
                       sharedData.hysteresis);

            } else if (strncmp(cmd, "hyst", 4) == 0 && cmd[4] != '\0') {
                // "hyst2" → hysteresis = 2 °C
                float h = atof(cmd + 4);
                if (h >= 0.1f && h <= 10.0f) {
                    sharedData.hysteresis = h;
                    printf("[CTRL] Hysteresis set to %.1f degC\n", h);
                } else {
                    printf("[CTRL] Invalid hysteresis (0.1-10): %.1f\n", h);
                }

            } else if (strncmp(cmd, "sp", 2) == 0 && cmd[2] != '\0') {
                // "sp35" → setpoint = 35 °C
                float sp = atof(cmd + 2);
                if (sp >= SETPOINT_MIN && sp <= SETPOINT_MAX) {
                    sharedData.setpoint = sp;
                    printf("[CTRL] SetPoint set to %.1f degC\n", sp);
                } else {
                    printf("[CTRL] SetPoint out of range (%.0f-%.0f)\n",
                           SETPOINT_MIN, SETPOINT_MAX);
                }
            }

            memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
            sharedData.serial_command_index = 0;
        }

        // ── Button: short press = SP+1°C, long press = SP-1°C ─────────────
        if (joystick != nullptr) {
            joystick->scan();
            uint32_t dur = joystick->getPressDuration();
            if (dur > 0) {
                float prev = sharedData.setpoint;
                if (dur >= 500) {
                    sharedData.setpoint -= SETPOINT_STEP;
                    if (sharedData.setpoint < SETPOINT_MIN)
                        sharedData.setpoint = SETPOINT_MIN;
                } else {
                    sharedData.setpoint += SETPOINT_STEP;
                    if (sharedData.setpoint > SETPOINT_MAX)
                        sharedData.setpoint = SETPOINT_MAX;
                }
                printf("[BTN] SetPoint: %.1f -> %.1f degC (%s press)\n",
                       prev, sharedData.setpoint,
                       dur >= 500 ? "long" : "short");
            }
        }

        if (emergencyStop || relay == nullptr) continue;

        // ── ON-OFF controller with hysteresis ─────────────────────────────
        float temp = sharedData.temperature;
        float sp   = sharedData.setpoint;
        float hyst = sharedData.hysteresis;
        bool  wasOn = sharedData.relayOn;
        bool  newOn = wasOn;

        if (temp < sp - hyst) {
            newOn = true;   // below lower bound → heat
        } else if (temp > sp + hyst) {
            newOn = false;  // above upper bound → stop
        }
        // within deadband: newOn = wasOn (no change)

        if (newOn != wasOn) {
            sharedData.relayOn = newOn;
            if (newOn) {
                relay->turnOn();
                if (led != nullptr) led->on();
                printf("[CTRL] Relay ON   SP:%.1f T:%.1f (below %.1f)\n",
                       sp, temp, sp - hyst);
            } else {
                relay->turnOff();
                if (led != nullptr) led->off();
                printf("[CTRL] Relay OFF  SP:%.1f T:%.1f (above %.1f)\n",
                       sp, temp, sp + hyst);
            }
        }

        // Signal display task
        semControlDisplay.give();
    }
}

// LCD line 1: "SP:30.0 T:25.4C"
// LCD line 2: "Relay:ON  H:1.0C"
// Serial Plotter: "SetPoint:30.0 Temp:25.4 Output:1"
void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISP] Task started (%dms)\n", DISPLAY_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        float sp   = sharedData.setpoint;
        float temp = sharedData.temperature;
        bool  on   = sharedData.relayOn;

        // ── LCD update ────────────────────────────────────────────────────
        if (semControlDisplay.take(pdMS_TO_TICKS(150))) {
            char line1[17], line2[17];
            // "SP:30.0 T:25.4C"  (16 chars)
            snprintf(line1, sizeof(line1), "SP:%-4.1f T:%-4.1fC", sp, temp);
            // "Rel:ON   H:1.0C"
            snprintf(line2, sizeof(line2), "Rel:%-3s  H:%.1fC",
                     on ? "ON" : "OFF", sharedData.hysteresis);
            updateLCD(line1, line2);
        }

        // ── Arduino Serial Plotter ────────────────────────────────────────
        printf("SetPoint:%.1f Temp:%.1f Output:%d\n", sp, temp, on ? 1 : 0);
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
