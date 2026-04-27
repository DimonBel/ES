#include "freertos_app/init/init.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "freertos_app/state/state.h"
#include "freertos_app/sync/sync.h"
#include "freertos_app/tasks/tasks.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

void setupApplication() {
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    printf("\n==========================================\n");
    printf("=== LAB 6.2.1 - VARIANT B              ===\n");
    printf("=== ON-OFF Control with Hysteresis      ===\n");
    printf("=== Motor Position Control via L298N    ===\n");
    printf("==========================================\n");
    printf("Hardware wiring:\n");
    printf("  SetPoint : Potentiometer GPIO %d\n", POTENTIOMETER_PIN);
    printf("  Value    : Joystick X    GPIO %d\n", JOYSTICK_X_PIN);
    printf("  L298N IN1: GPIO %d\n", MOTOR_IN1_PIN);
    printf("  L298N IN2: GPIO %d\n", MOTOR_IN2_PIN);
    printf("  L298N ENA: GPIO %d (PWM, 50%% fixed)\n", MOTOR_ENA_PIN);
    printf("  LCD      : I2C SDA=%d SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("Serial commands:\n");
    printf("  status   – print current state\n");
    printf("  stop     – emergency stop motor\n");
    printf("  run      – re-enable control after stop\n");
    printf("  hystX    – set hysteresis to X%% (e.g. hyst8)\n");
    printf("==========================================\n\n");

    // Motor (L298N)
    motor = new Motor(MOTOR_IN1_PIN, MOTOR_IN2_PIN, MOTOR_ENA_PIN);
    motor->begin();
    printf("Motor (L298N) initialized – STOP\n");

    // Potentiometer – SetPoint input
    potentiometer = new Potentiometer(POTENTIOMETER_PIN);
    potentiometer->begin();
    printf("Potentiometer initialized (SetPoint GPIO %d)\n", POTENTIOMETER_PIN);

    // Joystick – Value / simulated position sensor
    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("Joystick X initialized (Value GPIO %d)\n", JOYSTICK_X_PIN);

    // Status LED
    led = new Led(LED_PIN);
    led->begin();
    printf("LED initialized (GPIO %d)\n", LED_PIN);

    // LCD
    printf("Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Lab 6.2.1 Ready");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("ON-OFF Hyst Ctrl");
        kernel_primitives::delayMs(100);
        printf("LCD initialized\n");
    } else {
        printf("ERROR: Failed to create LCD!\n");
    }

    // Shared data
    sharedData.serial_command_received = false;
    sharedData.serial_command_index    = 0;
    memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
    sharedData.setpoint   = 50;
    sharedData.value      = 50;
    sharedData.output     = 0;
    sharedData.hysteresis = DEFAULT_HYSTERESIS;
    sharedData.motorSpeed = MOTOR_SATURATION_SPEED;

    if (!initSyncPrimitives()) {
        printf("ERROR: Failed to create sync primitives!\n");
        while (1) {}
    }

    printf("Creating FreeRTOS tasks...\n");
    if (createApplicationTasks()) {
        printf("=== FREERTOS SCHEDULER STARTED ===\n");
        printf("  Acquisition  P%d  %dms\n", TASK_PRIORITY_ACQUISITION, ACQUISITION_PERIOD_MS);
        printf("  OnOffControl P%d  %dms\n", TASK_PRIORITY_CONTROL,     CONTROL_PERIOD_MS);
        printf("  Display      P%d  %dms\n", TASK_PRIORITY_DISPLAY,      DISPLAY_PERIOD_MS);
        printf("==========================================\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1) {}
    }
}

}
