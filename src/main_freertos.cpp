#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "serial_stdio/serial_stdio.h"
#include "led/led.h"
#include "lcd/lcd.h"
#include "joystick/joystick.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

// ========== PINI ==========
static const uint8_t JOYSTICK_X_PIN = A0;
static const uint8_t JOYSTICK_Y_PIN = A1;
static const uint8_t JOYSTICK_SW_PIN = 3;
static const uint8_t LED_RED_PIN = 12;
static const uint8_t LED_GREEN_PIN = 11;
static const uint8_t LED_YELLOW_PIN = 13;

// ========== STRUCTURI SHARED DATA ==========
typedef struct {
    uint32_t press_duration;
    bool new_press_detected;
    bool button_pressed;
    uint8_t task_state;  // 0=ready, 1=pressed, 2=result, 3=wait
} SharedData;

// ========== VARIABILE GLOBALE ==========
static Led *ledG = nullptr;
static Led *ledR = nullptr;
static Led *ledY = nullptr;
static LcdI2c *lcd = nullptr;
static Joystick *joystick = nullptr;

// MECANISME DE SINCRONIZARE
static SemaphoreHandle_t xMutexLCD = nullptr;      // Mutex pentru acces LCD
static SemaphoreHandle_t xSemaphorePress = nullptr; // Binary semaphore pentru semnalizare press
static SemaphoreHandle_t xSemaphoreRelease = nullptr; // Binary semaphore pentru semnalizare release

// DATE PARTAJATE (PROTEJATE DE MUTEX)
static SharedData shared_data = {0, false, false, 0};

// ========== TASK PARAMETERS ==========
#define TASK_STACK_SIZE 256
#define TASK_PRIORITY_DETECT (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_DISPLAY (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_LED (tskIDLE_PRIORITY + 1)

// ========== TASK DECLARATIONS ==========
void vTaskDetect(void *pvParameters);
void vTaskDisplay(void *pvParameters);
void vTaskLED(void *pvParameters);

// ========== HELPER FUNCTIONS ==========
void updateLCD(const char *line1, const char *line2) {
    if (xSemaphoreTake(xMutexLCD, pdMS_TO_TICKS(100)) == pdTRUE) {
        lcd->clear();
        vTaskDelay(pdMS_TO_TICKS(50));
        lcd->setCursor(0, 0);
        lcd->print(line1);
        vTaskDelay(pdMS_TO_TICKS(50));
        lcd->setCursor(0, 1);
        lcd->print(line2);
        xSemaphoreGive(xMutexLCD);
    }
}

// ========== TASK 1: DETECT JOYSTICK PRESS ==========
void vTaskDetect(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms scan rate

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        joystick->scan();

        // Check button state
        if (joystick->isPressed()) {
            if (!shared_data.button_pressed) {
                // Button just pressed
                shared_data.button_pressed = true;
                shared_data.task_state = 1;

                // Signal other tasks
                xSemaphoreGive(xSemaphorePress);

                printf("[DETECT] Button pressed\n");
            }
        } else {
            if (shared_data.button_pressed) {
                // Button just released
                uint32_t duration = joystick->getPressDuration();
                shared_data.button_pressed = false;
                shared_data.press_duration = duration;
                shared_data.new_press_detected = true;
                shared_data.task_state = 2;

                // Signal release event
                xSemaphoreGive(xSemaphoreRelease);

                printf("[DETECT] Button released! Duration: %lu ms\n", duration);
            }
        }
    }
}

// ========== TASK 2: DISPLAY ON LCD ==========
void vTaskDisplay(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Wait for press event
        if (xSemaphoreTake(xSemaphorePress, pdMS_TO_TICKS(100)) == pdTRUE) {
            updateLCD("Pressing...", "Button");
        }

        // Wait for release event
        if (xSemaphoreTake(xSemaphoreRelease, pdMS_TO_TICKS(100)) == pdTRUE) {
            char line1[16];
            char line2[16];

            if (shared_data.press_duration > 500) {
                snprintf(line1, sizeof(line1), "Green LED:");
            } else {
                snprintf(line1, sizeof(line1), "Red LED:");
            }
            snprintf(line2, sizeof(line2), "%lu ms", shared_data.press_duration);

            updateLCD(line1, line2);

            // Wait 5 seconds then show "Press Joystick"
            vTaskDelay(pdMS_TO_TICKS(5000));
            updateLCD("Press Joystick", "Button");

            shared_data.task_state = 0;
            shared_data.new_press_detected = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ========== TASK 3: CONTROL LEDs ==========
void vTaskLED(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Wait for press event
        if (xSemaphoreTake(xSemaphorePress, pdMS_TO_TICKS(100)) == pdTRUE) {
            ledR->off();
            ledG->off();
            ledY->on();
            printf("[LED] Yellow ON, others OFF\n");
        }

        // Wait for release event
        if (xSemaphoreTake(xSemaphoreRelease, pdMS_TO_TICKS(100)) == pdTRUE) {
            ledY->off();
            ledR->off();
            ledG->off();

            if (shared_data.press_duration > 500) {
                ledG->on();
                printf("[LED] Green ON (duration > 500ms)\n");
            } else {
                ledR->on();
                printf("[LED] Red ON (duration <= 500ms)\n");
            }
        }

        // Check if 5 seconds passed to turn off LEDs
        if (shared_data.task_state == 0) {
            ledR->off();
            ledG->off();
            ledY->off();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ========== SETUP ==========
void setup() {
    SerialStdio::begin(9600);
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n=== LAB 3.2.2 - FreeRTOS JOYSTICK & LED ===\n");
    printf("Joystick: X=A0, Y=A1, SW=D3\n");
    printf("LED R: %d, LED G: %d, LED Y: %d\n", LED_RED_PIN, LED_GREEN_PIN, LED_YELLOW_PIN);
    printf("LCD: I2C (0x27)\n");
    printf("==========================================\n");

    // Initialize LEDs
    ledG = new Led(LED_GREEN_PIN);
    ledR = new Led(LED_RED_PIN);
    ledY = new Led(LED_YELLOW_PIN);
    ledG->begin();
    ledR->begin();
    ledY->begin();
    ledG->off();
    ledR->off();
    ledY->off();
    printf("LEDs initialized\n");

    // Initialize Joystick
    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();
    printf("Joystick initialized\n");

    // Test LEDs
    printf("Testing LEDs...\n");
    ledG->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledG->off();
    ledR->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledR->off();
    ledY->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledY->off();
    printf("LED test complete\n");

    // Initialize LCD
    printf("Initializing LCD...\n");
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd = new LcdI2c(0x27, 16, 2);
    lcd->begin();
    vTaskDelay(pdMS_TO_TICKS(500));
    lcd->setCursor(0, 0);
    lcd->print("Press Joystick");
    vTaskDelay(pdMS_TO_TICKS(100));
    lcd->setCursor(0, 1);
    lcd->print("Button");
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("LCD initialized\n");

    // Create synchronization objects
    xMutexLCD = xSemaphoreCreateMutex();
    xSemaphorePress = xSemaphoreCreateBinary();
    xSemaphoreRelease = xSemaphoreCreateBinary();

    if (xMutexLCD == nullptr || xSemaphorePress == nullptr || xSemaphoreRelease == nullptr) {
        printf("ERROR: Failed to create semaphores/mutex!\n");
        while (1);
    }

    // Create tasks
    printf("Creating FreeRTOS tasks...\n");

    BaseType_t xReturned1 = xTaskCreate(
        vTaskDetect,
        "Detect",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_DETECT,
        nullptr
    );

    BaseType_t xReturned2 = xTaskCreate(
        vTaskDisplay,
        "Display",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_DISPLAY,
        nullptr
    );

    BaseType_t xReturned3 = xTaskCreate(
        vTaskLED,
        "LED",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY_LED,
        nullptr
    );

    if (xReturned1 == pdPASS && xReturned2 == pdPASS && xReturned3 == pdPASS) {
        printf("=== FREE-RTOS SCHEDULER STARTED ===\n");
        printf("Tasks running:\n");
        printf("  - Detect (priority %d)\n", TASK_PRIORITY_DETECT);
        printf("  - Display (priority %d)\n", TASK_PRIORITY_DISPLAY);
        printf("  - LED (priority %d)\n", TASK_PRIORITY_LED);
        printf("=====================================\n");
        printf("Press joystick button...\n\n");
    } else {
        printf("ERROR: Failed to create tasks!\n");
        while (1);
    }

    // Start scheduler
    vTaskStartScheduler();

    // Should never reach here
    printf("ERROR: Scheduler returned!\n");
    while (1);
}

// ========== LOOP (not used in FreeRTOS) ==========
void loop() {
    // Empty - FreeRTOS handles everything
}