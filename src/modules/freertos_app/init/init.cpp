#include "freertos_app/init/init.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "freertos_app/tasks/tasks.h"
#include "kernel_primitives/task/task.h"
#include "servo/servo.h"
#include "potentiometer/potentiometer.h"

namespace freertos_app::internal {

void setupApplication() {
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    printf("\n==========================================\n");
    printf("=== LAB 4.2 - VARIANT C (100% Points) ===\n");
    printf("=== Dual Actuator Control System ===\n");
    printf("==========================================\n");
    printf("\nBinary Actuator (Relay):\n");
    printf("  - Pin: GPIO %d\n", ACTUATOR_PIN);
    printf("  - Control: Serial commands, Button\n");
    printf("\nAnalog Actuator (Servo):\n");
    printf("  - Pin: GPIO %d (PWM)\n", SERVO_PIN);
    printf("  - Control: Potentiometer (GPIO %d)\n", POTENTIOMETER_PIN);
    printf("\nDisplay:\n");
    printf("  - LCD: I2C SDA=%d, SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("==========================================\n");
    printf("\nBinary Actuator Commands (Serial):\n");
    printf("  'on'     - Turn relay ON\n");
    printf("  'off'    - Turn relay OFF\n");
    printf("  'toggle' - Toggle relay state\n");
    printf("  'status' - Display current state\n");
    printf("\nAnalog Actuator (Servo):\n");
    printf("  - Rotate potentiometer to control speed (0-100%%)\n");
    printf("  - Servo angle: 0-180 degrees\n");
    printf("==========================================\n\n");

    // Initialize Actuator (Relay)
    actuator = new Actuator(ACTUATOR_PIN);
    actuator->begin();
    printf("Binary actuator (relay) initialized (OFF)\n");

    // Initialize Servo (Analog actuator)
    servo = new Servo(SERVO_PIN);
    servo->begin();
    printf("Analog actuator (servo) initialized (center position)\n");

    // Initialize Potentiometer (Speed control)
    potentiometer = new Potentiometer(POTENTIOMETER_PIN);
    potentiometer->begin();
    printf("Potentiometer initialized for speed control\n");

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
        lcd->print("Dual Actuator");
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

    // Initialize servo data
    sharedData.servo_speed = 50;          // Start at 50%
    sharedData.servo_angle = 90;          // Center position (90 degrees)
    sharedData.potentiometer_raw = 0;
    sharedData.potentiometer_percent = 0;
    sharedData.servo_enabled = true;
    sharedData.servo_command_time = 0;

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
        printf("  - Servo Control (priority %d, period: %dms)\n", TASK_PRIORITY_SERVO, SERVO_CONTROL_PERIOD_MS);
        printf("  - Display (priority %d, period: %dms)\n", TASK_PRIORITY_DISPLAY, DISPLAY_PERIOD_MS);
        printf("==========================================\n");
        printf("Dual actuator control system active...\n\n");
        printf("Binary actuator: Ready for commands\n");
        printf("Analog actuator: Ready for potentiometer input\n\n");
        printf("Waiting for input...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }
}

}
