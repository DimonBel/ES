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

    uint32_t lastButtonToggleTime = 0; // Pentru protecție împotriva "double-click"
    bool lastJoystickState = false;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (actuator == nullptr || signalConditioner == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        uint32_t currentTime = millis();
        if (joystick != nullptr) {
            joystick->scan(); 
            bool joyPressNow = joystick->isPressed();
            
            // Folosim isPressed() în loc de wasPressed() pentru a avea control complet aici
            if (joyPressNow && !lastJoystickState) {
                // Aplicăm cooldown de 250ms (debouncing agresiv)
                if ((currentTime - lastButtonToggleTime) > 250) {
                    sharedData.actuator_command = !sharedData.actuator_command; // Togglem starea
                    sharedData.actuator_command_time = currentTime;
                    lastButtonToggleTime = currentTime;
                    printf("[ACTUATOR_CTRL] Joystick explicitly toggled to: %s\n", sharedData.actuator_command ? "ON" : "OFF");
                }
            }
            lastJoystickState = joyPressNow;
        }

        // Check for serial commands
        if (sharedData.serial_command_received) {
            sharedData.serial_command_received = false;
            sharedData.actuator_command_time = currentTime;

            const char* cmd = sharedData.serial_command_buffer;
            // printf("[ACTUATOR_CTRL] Serial command: %s\n", cmd);

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

        // Control actuator and LED based on conditioned signal
        if (conditionedSignal != sharedData.actuator_state) {
            // State changed
            sharedData.actuator_state = conditionedSignal;
            sharedData.actuator_toggle_count++;

            if (conditionedSignal) {
                actuator->turnOn();
                if (led != nullptr) led->on();
            } else {
                actuator->turnOff();
                if (led != nullptr) led->off();
            }
        }
    }
}

void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISPLAY] Task started (period: %dms)\n", DISPLAY_PERIOD_MS);
    
    // Contor pentru a raporta o dată la 10 secunde / Report counter
    uint32_t reportCounter = 0;
    const uint32_t reportsPer10Seconds = 10000 / DISPLAY_PERIOD_MS;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];

        // Wait for actuator display signal
        if (semActuatorDisplay.take(pdMS_TO_TICKS(100))) {
            // Display both actuator (binary) and servo (analog) states
            const char* actuatorStateStr = sharedData.actuator_state ? "ON" : "OFF";
            const char* actuatorCmdStr = sharedData.actuator_command ? "ON" : "OFF";

            // Display format: "Act:ON Serv:50%" and "Cmd:ON Tog:123"
            snprintf(line1, sizeof(line1), "Act:%s Srv:%d%%", 
                     actuatorStateStr, sharedData.servo_speed);
            snprintf(line2, sizeof(line2), "Cmd:%s Tog:%lu", 
                     actuatorCmdStr, sharedData.actuator_toggle_count);

            updateLCD(line1, line2);
            
            // Afisare raport la fiecare 10 secunde
            reportCounter++;
            if (reportCounter >= reportsPer10Seconds) {
                printf("\n==========================================\n");
                printf("       [10s DUAL ACTUATOR STATUS REPORT]\n");
                printf(" Binary Actuator (Relay):\n");
                printf("  - Command: %s\n", actuatorCmdStr);
                printf("  - State:   %s\n", actuatorStateStr);
                printf("  - Toggles: %lu\n", sharedData.actuator_toggle_count);
                printf("\n Analog Actuator (Servo):\n");
                printf("  - Speed:     %d%%\n", sharedData.servo_speed);
                printf("  - Angle:     %d degrees\n", sharedData.servo_angle);
                printf("  - Potentiometer: %d%%\n", sharedData.potentiometer_percent);
                printf("==========================================\n\n");
                reportCounter = 0; // reset counter
            }
        }
    }
}

void vTaskServoControl(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(SERVO_CONTROL_PERIOD_MS);

    printf("[SERVO_CTRL] Task started (period: %dms)\n", SERVO_CONTROL_PERIOD_MS);

    uint32_t lastServoUpdate = 0; // For cooldown

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (servo == nullptr || potentiometer == nullptr) {
            kernel_primitives::delayMs(100);
            continue;
        }

        uint32_t currentTime = millis();

        // Read potentiometer value
        potentiometer->scan();
        int potPercent = potentiometer->getPercentage();
        int potRaw = potentiometer->getRaw();

        // Update shared data
        sharedData.potentiometer_raw = potRaw;
        sharedData.potentiometer_percent = potPercent;

        // Synchronize servo with relay state
        sharedData.servo_enabled = sharedData.actuator_state;

        if (!sharedData.servo_enabled) {
            servo->stop();
            actuator->setSpeed(0);
        } else {
            // Update DC motor speed (Actuator)
            actuator->setSpeed(potPercent);

            // Apply cooldown to prevent rapid changes for Servo
            if ((currentTime - lastServoUpdate) >= SERVO_COOLDOWN_MS) {
                // Only update servo if potentiometer value changed significantly
                if (abs(potPercent - sharedData.servo_speed) > 2) { // Threshold: 2%
                    sharedData.servo_speed = potPercent;
                    sharedData.servo_command_time = currentTime;
                    
                    // Map speed to angle (0-100% -> 0-180 degrees)
                    uint8_t angle = (potPercent * 180) / 100;
                    sharedData.servo_angle = angle;

                    // Control servo with ramping (smooth movement)
                    servo->moveTo(angle, 2); // Ramp speed: 2 degrees per step

                    lastServoUpdate = currentTime;
                    printf("[CONTROL] Potentiometer: %d%% -> Angle: %d deg, Motor Speed: %d%%\n", 
                        potPercent, angle, potPercent);
                }
            }
            // Update servo for smooth movement
            servo->update();
        }

        // Signal display task to update
        semActuatorDisplay.give();
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

    bool servoCtrlCreated = kernel_primitives::createTask(
        vTaskServoControl,
        "ServoCtrl",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_SERVO,
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

    return actuatorCtrlCreated && signalCondCreated && servoCtrlCreated && displayCreated;
}

}
