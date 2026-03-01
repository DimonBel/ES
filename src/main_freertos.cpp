#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "led/led.h"
#include "lcd/lcd.h"
#include "joystick/joystick.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ========== PINI ESP32 ==========
static const uint8_t JOYSTICK_X_PIN = 34;  // ADC1_CH6 (input only)
static const uint8_t JOYSTICK_Y_PIN = 35;  // ADC1_CH7 (input only)
static const uint8_t JOYSTICK_SW_PIN = 2;   // GPIO 2
static const uint8_t LED_RED_PIN = 12;     // GPIO 12
static const uint8_t LED_GREEN_PIN = 14;   // GPIO 14
static const uint8_t LED_YELLOW_PIN = 13;  // GPIO 13

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
static SemaphoreHandle_t xSemaphorePressDisplay = nullptr;
static SemaphoreHandle_t xSemaphoreReleaseDisplay = nullptr;
static SemaphoreHandle_t xSemaphorePressLED = nullptr;
static SemaphoreHandle_t xSemaphoreReleaseLED = nullptr;

// DATE PARTAJATE (PROTEJATE DE MUTEX)
static SharedData shared_data = {0, false, false, 0};

// ========== TASK PARAMETERS ==========
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY_DETECT (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_DISPLAY (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_LED (tskIDLE_PRIORITY + 1)

// ========== TASK DECLARATIONS ==========
void vTaskDetect(void *pvParameters);
void vTaskDisplay(void *pvParameters);
void vTaskLED(void *pvParameters);

// ========== HELPER FUNCTIONS ==========
void updateLCD(const char *line1, const char *line2) {
    if (lcd == nullptr) return;
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

        if (joystick == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        joystick->scan();

        // Check button state
        if (joystick->isPressed()) {
            if (!shared_data.button_pressed) {
                // Button just pressed
                shared_data.button_pressed = true;
                shared_data.task_state = 1;

                // Signal other tasks
                xSemaphoreGive(xSemaphorePressDisplay);
                xSemaphoreGive(xSemaphorePressLED);

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
                xSemaphoreGive(xSemaphoreReleaseDisplay);
                xSemaphoreGive(xSemaphoreReleaseLED);

                printf("[DETECT] Button released! Duration: %lu ms\n", duration);
            }
        }
    }
}

// ========== TASK 2: DISPLAY ON LCD ==========
void vTaskDisplay(void *pvParameters) {
    TickType_t resultDeadline = 0;
    bool resultVisible = false;

    for (;;) {
        // Wait for press event
        if (xSemaphoreTake(xSemaphorePressDisplay, pdMS_TO_TICKS(100)) == pdTRUE) {
            updateLCD("Pressing...", "Button");
            resultVisible = false;
        }

        // Wait for release event
        if (xSemaphoreTake(xSemaphoreReleaseDisplay, pdMS_TO_TICKS(100)) == pdTRUE) {
            char line1[16];
            char line2[16];

            if (shared_data.press_duration > 500) {
                snprintf(line1, sizeof(line1), "Green LED:");
            } else {
                snprintf(line1, sizeof(line1), "Red LED:");
            }
            snprintf(line2, sizeof(line2), "%lu ms", shared_data.press_duration);

            updateLCD(line1, line2);
            resultDeadline = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
            resultVisible = true;
        }

        if (resultVisible && xTaskGetTickCount() >= resultDeadline) {
            updateLCD("Press Joystick", "Button");
            shared_data.task_state = 0;
            shared_data.new_press_detected = false;
            resultVisible = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ========== TASK 3: CONTROL LEDs ==========
void vTaskLED(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Wait for press event
        if (xSemaphoreTake(xSemaphorePressLED, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (ledR) ledR->off();
            if (ledG) ledG->off();
            if (ledY) ledY->on();
            printf("[LED] Yellow ON, others OFF\n");
        }

        // Wait for release event
        if (xSemaphoreTake(xSemaphoreReleaseLED, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (ledY) ledY->off();
            if (ledR) ledR->off();
            if (ledG) ledG->off();

            if (shared_data.press_duration > 500) {
                if (ledG) ledG->on();
                printf("[LED] Green ON (duration > 500ms)\n");
            } else {
                if (ledR) ledR->on();
                printf("[LED] Red ON (duration <= 500ms)\n");
            }
        }

        // Check if 5 seconds passed to turn off LEDs
        if (shared_data.task_state == 0) {
            if (ledR) ledR->off();
            if (ledG) ledG->off();
            if (ledY) ledY->off();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
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
    if (ledG) { ledG->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledG->off(); }
    if (ledR) { ledR->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledR->off(); }
    if (ledY) { ledY->on(); vTaskDelay(pdMS_TO_TICKS(100)); ledY->off(); }
    printf("LED test complete\n");

    // Initialize LCD
    printf("Initializing LCD...\n");
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        vTaskDelay(pdMS_TO_TICKS(500));
        lcd->setCursor(0, 0);
        lcd->print("Press Joystick");
        vTaskDelay(pdMS_TO_TICKS(100));
        lcd->setCursor(0, 1);
        lcd->print("Button");
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("LCD initialized\n");
    } else {
        printf("ERROR: Failed to create LCD object!\n");
    }

    // Create synchronization objects
    xMutexLCD = xSemaphoreCreateMutex();
    xSemaphorePressDisplay = xSemaphoreCreateBinary();
    xSemaphoreReleaseDisplay = xSemaphoreCreateBinary();
    xSemaphorePressLED = xSemaphoreCreateBinary();
    xSemaphoreReleaseLED = xSemaphoreCreateBinary();

    if (xMutexLCD == nullptr ||
        xSemaphorePressDisplay == nullptr ||
        xSemaphoreReleaseDisplay == nullptr ||
        xSemaphorePressLED == nullptr ||
        xSemaphoreReleaseLED == nullptr) {
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

}

// ========== LOOP (not used in FreeRTOS) ==========
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}