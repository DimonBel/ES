#include "freertos_app/tasks/tasks.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

// ─── Finite State Machine ─────────────────────────────────────────────────────
//
//   ┌───────────────────────────────────────────────────────┐
//   │              AUTOMAT FINIT – Button/LED               │
//   ├──────────┬─────────────────┬─────────────────────────┤
//   │  State   │     Event       │  Next State  │  Action   │
//   ├──────────┼─────────────────┼──────────────┼──────────-┤
//   │ LED_OFF  │  button press   │   LED_ON     │  LED on   │
//   │ LED_ON   │  button press   │   LED_OFF    │  LED off  │
//   └──────────┴─────────────────┴──────────────┴───────────┘
//
// Input event: debounced button press (rising edge)
// Initial state: LED_OFF

static bool fsmButtonPressed() {
    if (joystick == nullptr) return false;
    joystick->scan();
    if (!joystick->wasPressed()) return false;
    uint32_t now = millis();
    if (now - sharedData.lastPressTime < DEBOUNCE_MS) return false;
    sharedData.lastPressTime = now;
    return true;
}

static void fsmRun() {
    if (!fsmButtonPressed()) return;

    switch (sharedData.ledState) {

        case LedState::OFF:
            sharedData.ledState = LedState::ON;
            led->on();
            printf("[FSM] Transition: OFF --> ON  (LED on)\n");
            break;

        case LedState::ON:
            sharedData.ledState = LedState::OFF;
            led->off();
            printf("[FSM] Transition: ON  --> OFF (LED off)\n");
            break;
    }
}

// ─── FSM Task (50 ms) 
void vTaskFSM(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(FSM_PERIOD_MS);

    printf("[FSM] Task started (%dms) – initial state: OFF\n", FSM_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        if (led == nullptr) continue;
        fsmRun();
    }
}

// ─── Display Task (500 ms) ────────────────────────────────────────────────────
// Refreshes LCD and serial with current LED state.
void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(DISPLAY_PERIOD_MS);

    printf("[DISP] Task started (%dms)\n", DISPLAY_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        const char *stateStr = (sharedData.ledState == LedState::ON) ? "ON " : "OFF";

        char line1[17];
        char line2[17];
        snprintf(line1, sizeof(line1), "FSM Button-LED");
        snprintf(line2, sizeof(line2), "State: %-3s", stateStr);

        updateLCD(line1, line2);
        printf("[DISP] LED State: %s\n", stateStr);
    }
}

// ─── Task creation ────────────────────────────────────────────────────────────
bool createApplicationTasks() {
    bool ok = true;

    ok &= kernel_primitives::createTask(
        vTaskFSM, "FSM",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_FSM, nullptr);

    ok &= kernel_primitives::createTask(
        vTaskDisplay, "Display",
        TASK_STACK_SIZE, nullptr,
        TASK_PRIORITY_DISPLAY, nullptr);

    return ok;
}

}
