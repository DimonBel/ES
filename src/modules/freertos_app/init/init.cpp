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
    printf("=== LAB 5.1 - VARIANT A                ===\n");
    printf("=== ON-OFF Temperature Control          ===\n");
    printf("=== with Hysteresis (DS18B20 + Relay)   ===\n");
    printf("==========================================\n");
    printf("Hardware wiring:\n");
    printf("  DHT11 DATA   : GPIO %d\n", DHT11_PIN);
    printf("  Relay IN     : GPIO %d\n", RELAY_PIN);
    printf("  Button SW    : GPIO %d  (joystick SW)\n", JOYSTICK_SW_PIN);
    printf("  LED          : GPIO %d\n", LED_PIN);
    printf("  LCD I2C      : SDA=%d  SCL=%d  (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("Button behavior:\n");
    printf("  Short press (<500ms) : SetPoint +1 degC\n");
    printf("  Long  press (>=500ms): SetPoint -1 degC\n");
    printf("Serial commands:\n");
    printf("  status       – print current state\n");
    printf("  hystX        – set hysteresis to X deg (e.g. hyst2)\n");
    printf("  spX          – set setpoint to X degC  (e.g. sp35)\n");
    printf("==========================================\n\n");

    // DHT11 temperature sensor
    dht11Sensor = new Dht11Sensor(DHT11_PIN);
    dht11Sensor->begin();
    printf("[INIT] DHT11 initialized on GPIO %d\n", DHT11_PIN);

    // Relay (Actuator)
    relay = new Actuator(RELAY_PIN);
    relay->begin();
    printf("[INIT] Relay initialized on GPIO %d – OFF\n", RELAY_PIN);

    // Joystick button only (X/Y not connected – dummy ADC pins)
    joystick = new Joystick(34, 35, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("[INIT] Button initialized on GPIO %d\n", JOYSTICK_SW_PIN);

    // Status LED (relay state indicator)
    led = new Led(LED_PIN);
    led->begin();
    printf("[INIT] LED initialized on GPIO %d\n", LED_PIN);

    // LCD
    printf("[INIT] Initializing LCD I2C...\n");
    kernel_primitives::delayMs(200);
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Lab 5.1 Ready");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("Temp ON-OFF Ctrl");
        kernel_primitives::delayMs(100);
        printf("[INIT] LCD OK\n");
    } else {
        printf("[INIT] ERROR: Failed to create LCD!\n");
    }

    // Shared data
    sharedData.serial_command_received = false;
    sharedData.serial_command_index    = 0;
    memset(sharedData.serial_command_buffer, 0, sizeof(sharedData.serial_command_buffer));
    sharedData.setpoint    = DEFAULT_SETPOINT;
    sharedData.temperature = 0.0f;
    sharedData.relayOn     = false;
    sharedData.hysteresis  = DEFAULT_HYSTERESIS;

    if (!initSyncPrimitives()) {
        printf("[INIT] ERROR: Failed to create sync primitives!\n");
        while (1) {}
    }

    printf("[INIT] Creating FreeRTOS tasks...\n");
    if (createApplicationTasks()) {
        printf("=== FreeRTOS SCHEDULER STARTED ===\n");
        printf("  Acquisition  P%d  %dms\n", TASK_PRIORITY_ACQUISITION, ACQUISITION_PERIOD_MS);
        printf("  OnOffControl P%d  %dms\n", TASK_PRIORITY_CONTROL,     CONTROL_PERIOD_MS);
        printf("  Display      P%d  %dms\n", TASK_PRIORITY_DISPLAY,      DISPLAY_PERIOD_MS);
        printf("==========================================\n\n");
    } else {
        printf("[INIT] ERROR: Failed to create tasks!\n");
        while (1) {}
    }
}

}
