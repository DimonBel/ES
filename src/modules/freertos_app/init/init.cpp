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

    printf("\n=====================================\n");
    printf("=== LAB 4.1 - Binary Actuator Control ===\n");
    printf("=====================================\n");
    printf("Actuator: Relay on pin %d\n", ACTUATOR_PIN);
    printf("Button: GPIO %d\n", BUTTON_PIN);
    printf("LCD: I2C SDA=%d, SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("=====================================\n");
    printf("\nControl Commands (Serial):\n");
    printf("  'on'  - Turn actuator ON\n");
    printf("  'off' - Turn actuator OFF\n");
    printf("  'toggle' - Toggle actuator state\n");
    printf("  'status' - Display current state\n");
    printf("=====================================\n\n");

    // Initialize Actuator (Relay)
    actuator = new Actuator(ACTUATOR_PIN);
    actuator->begin();
    printf("Actuator initialized (OFF)\n");

    // Initialize UI components
    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("Joystick initialized\n");

    led = new Led(LED_PIN);
    led->begin();
    printf("LED initialized (OFF)\n");

    // Initialize Signal Conditioner
    signalConditioner = new SignalConditioner();
    printf("Signal conditioner initialized\n");

    // Initialize LCD
    printf("Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Actuator Ctrl");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("System Ready");
        kernel_primitives::delayMs(100);
        printf("LCD initialized\n");
    } else {
        printf("ERROR: Failed to create LCD object!\n");
    }

    // Initialize shared data
    sharedData.actuator_command = false;
    sharedData.actuator_state = false;
    sharedData.actuator_conditioned = false;
    sharedData.actuator_command_time = 0;
    sharedData.actuator_toggle_count = 0;
    sharedData.serial_command_received = false;
    sharedData.serial_command_index = 0;
    memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));

    if (!initSyncPrimitives()) {
        printf("ERROR: Failed to create semaphores/mutex!\n");
        while (1);
    }

    printf("Creating FreeRTOS tasks...\n");

    if (createApplicationTasks()) {
        printf("=== FREE-RTOS SCHEDULER STARTED ===\n");
        printf("Tasks running:\n");
        printf("  - Actuator Control (priority %d, period: %dms)\n", TASK_PRIORITY_ACTUATOR, ACTUATOR_CONTROL_PERIOD_MS);
        printf("  - Signal Conditioning (priority %d)\n", TASK_PRIORITY_CONDITIONING);
        printf("  - Display (priority %d, period: %dms)\n", TASK_PRIORITY_DISPLAY, DISPLAY_PERIOD_MS);
        printf("=====================================\n");
        printf("Binary actuator control active...\n\n");
        printf("Waiting for commands...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }
}

}
