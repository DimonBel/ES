#include "freertos_app/init/init.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "freertos_app/tasks/tasks.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

void setupApplication() {
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    printf("\n==========================================\n");
    printf("=== LAB - FSM Button-LED Control        ===\n");
    printf("=== Finite State Machine (2 states)      ===\n");
    printf("==========================================\n");
    printf("Hardware wiring:\n");
    printf("  Button SW  : GPIO %d\n", JOYSTICK_SW_PIN);
    printf("  LED        : GPIO %d\n", LED_PIN);
    printf("  LCD I2C    : SDA=%d SCL=%d\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("Operation:\n");
    printf("  Press button -> toggle LED ON/OFF\n");
    printf("  Debounce     : %dms\n", DEBOUNCE_MS);
    printf("==========================================\n\n");

    // Button (joystick SW only – VRX/VRY unused)
    joystick = new Joystick(34, 35, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("[INIT] Button on GPIO %d\n", JOYSTICK_SW_PIN);

    // LED – start OFF (FSM initial state)
    led = new Led(LED_PIN);
    led->begin();
    led->off();
    printf("[INIT] LED on GPIO %d – initial state: OFF\n", LED_PIN);

    // LCD
    printf("[INIT] Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("FSM Button-LED");
        lcd->setCursor(0, 1);
        lcd->print("State: OFF");
        kernel_primitives::delayMs(100);
        printf("[INIT] LCD OK\n");
    }

    // Shared data
    sharedData.ledState      = LedState::OFF;
    sharedData.lastPressTime = 0;

    if (!initSyncPrimitives()) {
        printf("[INIT] ERROR: sync primitives failed!\n");
        while (1) {}
    }

    printf("[INIT] Creating FreeRTOS tasks...\n");
    if (createApplicationTasks()) {
        printf("=== FreeRTOS SCHEDULER STARTED ===\n");
        printf("  FSM     P%lu  %lums\n",
               (unsigned long)TASK_PRIORITY_FSM,     (unsigned long)FSM_PERIOD_MS);
        printf("  Display P%lu  %lums\n",
               (unsigned long)TASK_PRIORITY_DISPLAY, (unsigned long)DISPLAY_PERIOD_MS);
        printf("==========================================\n\n");
    } else {
        printf("[INIT] ERROR: task creation failed!\n");
        while (1) {}
    }
}

}
