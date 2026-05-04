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
    printf("=== LAB 5.2 - VARIANT A                ===\n");
    printf("=== PID Temperature Control             ===\n");
    printf("=== DHT11 + L298N heater                ===\n");
    printf("==========================================\n");
    printf("Hardware wiring:\n");
    printf("  DHT11 DATA : GPIO %d\n", DHT11_PIN);
    printf("  L298N IN1  : GPIO %d\n", MOTOR_IN1_PIN);
    printf("  L298N IN2  : GPIO %d\n", MOTOR_IN2_PIN);
    printf("  L298N ENA  : GPIO %d (PWM)\n", MOTOR_ENA_PIN);
    printf("  Button SW  : GPIO %d\n", JOYSTICK_SW_PIN);
    printf("  LED        : GPIO %d\n", LED_PIN);
    printf("  LCD I2C    : SDA=%d SCL=%d\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("Button:\n");
    printf("  Short (<500ms)   : SP +1 C\n");
    printf("  Long  (0.5-5s)   : SP -1 C\n");
    printf("  Hold  (>=5s)     : reset PID integral\n");
    printf("Serial (read-only):\n");
    printf("  status  – print current PID state\n");
    printf("==========================================\n\n");

    // DHT11
    dht11Sensor = new Dht11Sensor(DHT11_PIN);
    dht11Sensor->begin();

    // L298N motor driver (controls heater via PWM)
    motor = new Motor(MOTOR_IN1_PIN, MOTOR_IN2_PIN, MOTOR_ENA_PIN);
    motor->begin();
    motor->stop();
    printf("[INIT] L298N initialized – STOP\n");

    // Button
    joystick = new Joystick(34, 35, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("[INIT] Button initialized on GPIO %d\n", JOYSTICK_SW_PIN);

    // LED
    led = new Led(LED_PIN);
    led->begin();
    printf("[INIT] LED initialized on GPIO %d\n", LED_PIN);

    // LCD
    printf("[INIT] Initializing LCD...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Lab 5.2 PID Ctrl");
        lcd->setCursor(0, 1);
        lcd->print("DHT11 + L298N");
        kernel_primitives::delayMs(100);
        printf("[INIT] LCD OK\n");
    }

    // Shared data
    sharedData.serial_command_received = false;
    sharedData.serial_command_index    = 0;
    memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
    sharedData.setpoint    = DEFAULT_SETPOINT;
    sharedData.temperature = 0.0f;
    sharedData.pidOutput   = 0.0f;
    sharedData.kp          = DEFAULT_KP;
    sharedData.ki          = DEFAULT_KI;
    sharedData.kd          = DEFAULT_KD;
    sharedData.integral    = 0.0f;
    sharedData.prevError   = 0.0f;

    if (!initSyncPrimitives()) {
        printf("[INIT] ERROR: sync primitives failed!\n");
        while (1) {}
    }

    printf("[INIT] Creating FreeRTOS tasks...\n");
    if (createApplicationTasks()) {
        printf("=== FreeRTOS SCHEDULER STARTED ===\n");
        printf("  Acquisition P%d  %dms\n", TASK_PRIORITY_ACQUISITION, ACQUISITION_PERIOD_MS);
        printf("  PIDControl  P%d  %dms\n", TASK_PRIORITY_CONTROL,     CONTROL_PERIOD_MS);
        printf("  Display     P%d  %dms\n", TASK_PRIORITY_DISPLAY,      DISPLAY_PERIOD_MS);
        printf("==========================================\n\n");
    } else {
        printf("[INIT] ERROR: task creation failed!\n");
        while (1) {}
    }
}

}
