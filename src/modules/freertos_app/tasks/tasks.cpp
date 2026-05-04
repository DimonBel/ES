#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Task 1: Acquisition (1000 ms) ────────────────────────────────────────────
void vTaskAcquisition(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACQUISITION_PERIOD_MS);

    printf("[ACQ] Task started (%dms)\n", ACQUISITION_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (dht11Sensor != nullptr) {
            float t = dht11Sensor->readTemperature();
            if (t > -100.0f) {
                sharedData.temperature = t;
            } else {
                // Simulate for Wokwi / no-sensor demo
                float &temp = sharedData.temperature;
                if (sharedData.pidOutput > 5.0f) {
                    temp += 0.05f * (sharedData.pidOutput / 100.0f) * 10.0f;
                } else {
                    temp -= 0.05f;
                }
                temp = clampf(temp, 10.0f, 70.0f);
                printf("[ACQ] SIMULATED Temp=%.1f C\n", temp);
            }
        }
    }
}

// ─── Task 2: PID Control (100 ms) ─────────────────────────────────────────────
// PID output (0–100%) drives L298N ENA via PWM → variable heater power.
// Anti-windup: integral only accumulates when output is not saturated.
void vTaskPIDControl(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_PERIOD_MS);
    const float dt = CONTROL_PERIOD_MS / 1000.0f;  // 0.1 s

    printf("[PID] Task started (%dms)\n", CONTROL_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // ── Button ─────────────────────────────────────────────────────────
        // < 500 ms  → SP +1°C
        // 500–5000ms → SP -1°C
        // ≥ 5000 ms  → reset PID integral
        if (joystick != nullptr) {
            joystick->scan();
            uint32_t dur = joystick->getPressDuration();
            if (dur > 0) {
                if (dur >= 5000) {
                    sharedData.integral  = 0.0f;
                    sharedData.prevError = 0.0f;
                    printf("[BTN] PID integral RESET (held %lums)\n", (unsigned long)dur);
                } else if (dur >= 500) {
                    float prev = sharedData.setpoint;
                    sharedData.setpoint = clampf(sharedData.setpoint - SETPOINT_STEP,
                                                 SETPOINT_MIN, SETPOINT_MAX);
                    sharedData.integral = 0.0f;
                    printf("[BTN] SP-- : %.1f -> %.1f C\n", prev, sharedData.setpoint);
                } else {
                    float prev = sharedData.setpoint;
                    sharedData.setpoint = clampf(sharedData.setpoint + SETPOINT_STEP,
                                                 SETPOINT_MIN, SETPOINT_MAX);
                    sharedData.integral = 0.0f;
                    printf("[BTN] SP++ : %.1f -> %.1f C\n", prev, sharedData.setpoint);
                }
            }
        }

        // ── Serial: read-only status ──────────────────────────────────────
        if (sharedData.serial_command_received) {
            sharedData.serial_command_received = false;
            if (strcmp(sharedData.serial_command_buffer, "status") == 0) {
                printf("[PID] SP:%.1f T:%.1f Out:%.1f%% Kp:%.2f Ki:%.2f Kd:%.2f I:%.2f\n",
                       sharedData.setpoint, sharedData.temperature,
                       sharedData.pidOutput,
                       sharedData.kp, sharedData.ki, sharedData.kd,
                       sharedData.integral);
            }
            memset(sharedData.serial_command_buffer, 0,
                   sizeof(sharedData.serial_command_buffer));
            sharedData.serial_command_index = 0;
        }

        if (motor == nullptr) continue;

        // ── PID algorithm ─────────────────────────────────────────────────
        float sp    = sharedData.setpoint;
        float temp  = sharedData.temperature;
        float kp    = sharedData.kp;
        float ki    = sharedData.ki;
        float kd    = sharedData.kd;
        float error = sp - temp;

        float pTerm = kp * error;
        float dTerm = kd * (error - sharedData.prevError) / dt;

        // Compute tentative output (integral not yet updated)
        float rawOut = pTerm + sharedData.integral + dTerm;
        float output = clampf(rawOut, 0.0f, 100.0f);

        // Anti-windup: integrate only when output is not saturated
        if (output > 0.0f && output < 100.0f) {
            sharedData.integral += ki * error * dt;
            // Clamp integral itself to prevent runaway
            sharedData.integral = clampf(sharedData.integral, -100.0f, 100.0f);
        }

        sharedData.prevError = error;
        sharedData.pidOutput = output;

        // ── Actuate heater via L298N PWM ──────────────────────────────────
        if (output < 1.0f) {
            motor->stop();
            if (led != nullptr) led->off();
        } else {
            motor->forward((uint8_t)output);
            if (led != nullptr) led->on();
        }

        // Signal display task
        semControlDisplay.give();
    }
}

// ─── Task 3: Display (500 ms) ─────────────────────────────────────────────────
// LCD line 1: "SP:20.0 T:19.5C"
// LCD line 2: "PID:45%  E:-0.5"
// Serial Plotter: "SetPoint:20.0 Temp:19.5 Output:45.0"
void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISP] Task started (%dms)\n", DISPLAY_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        float sp   = sharedData.setpoint;
        float temp = sharedData.temperature;
        float out  = sharedData.pidOutput;
        float err  = sp - temp;

        // ── LCD ───────────────────────────────────────────────────────────
        if (semControlDisplay.take(pdMS_TO_TICKS(150))) {
            char line1[17], line2[17];
            snprintf(line1, sizeof(line1), "SP:%-4.1f T:%-4.1fC", sp, temp);
            snprintf(line2, sizeof(line2), "PID:%-3.0f%%  E:%-+4.1f", out, err);
            updateLCD(line1, line2);
        }

        // ── Arduino Serial Plotter ────────────────────────────────────────
        printf("SetPoint:%.1f Temp:%.1f Output:%.1f\n", sp, temp, out);
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
        vTaskPIDControl, "PIDCtrl",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_CONTROL, nullptr);

    ok &= kernel_primitives::createTask(
        vTaskDisplay, "Display",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_DISPLAY, nullptr);

    return ok;
}

}
