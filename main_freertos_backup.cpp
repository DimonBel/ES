/**
 * Laborator 3.2 - Monitorizare durată apăsare buton
 * Buton: 1 de pe keypad
 * LED: Verde=3, Rosu=2, Galben=13
 */

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include "serial_stdio/serial_stdio.h"
#include "led/led.h"

// ========== PINI ==========
static const uint8_t ROW_PINS[4] = {4, 5, 6, 7};
static const uint8_t COL_PINS[4] = {8, 9, 10, 11};
static const uint8_t LED_GREEN_PIN = 3;
static const uint8_t LED_RED_PIN = 2;
static const uint8_t LED_YELLOW_PIN = 13;

// ========== CONFIG ==========
#define SHORT_PRESS_MS 500
#define REPORT_PERIOD_MS 10000

// ========== VARIABILE GLOBALE ==========
typedef struct {
    uint32_t total;
    uint32_t short_cnt;
    uint32_t long_cnt;
    uint32_t short_dur;
    uint32_t long_dur;
} Stats;

static Stats g_stats;
static SemaphoreHandle_t g_mutex;
static QueueHandle_t g_queue;

typedef struct {
    uint32_t duration;
    bool is_short;
} PressEvent;

static Led *ledG = nullptr;
static Led *ledR = nullptr;
static Led *ledY = nullptr;

static bool btn_pressed = false;
static uint32_t btn_start = 0;

// ========== FUNCȚII ==========

bool scanBtn1() {
    pinMode(ROW_PINS[0], OUTPUT);
    digitalWrite(ROW_PINS[0], LOW);
    pinMode(COL_PINS[0], INPUT_PULLUP);
    bool pressed = (digitalRead(COL_PINS[0]) == LOW);
    digitalWrite(ROW_PINS[0], HIGH);
    return pressed;
}

void blinkY(uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        ledY->toggle();
        vTaskDelay(pdMS_TO_TICKS(80));
        ledY->toggle();
        vTaskDelay(pdMS_TO_TICKS(80));
    }
    ledY->off();
}

// ========== TASK 1: Detectare ==========
void vTaskBtn(void *pv) {
    bool last = false;
    
    for (;;) {
        bool curr = scanBtn1();
        
        if (curr && !last) {
            btn_pressed = true;
            btn_start = millis();
            ledY->on();
        }
        else if (!curr && last && btn_pressed) {
            uint32_t dur = millis() - btn_start;
            bool is_short = (dur < SHORT_PRESS_MS);
            
            if (xSemaphoreTake(g_mutex, portMAX_DELAY)) {
                g_stats.total++;
                if (is_short) {
                    g_stats.short_cnt++;
                    g_stats.short_dur += dur;
                } else {
                    g_stats.long_cnt++;
                    g_stats.long_dur += dur;
                }
                xSemaphoreGive(g_mutex);
            }
            
            PressEvent ev;
            ev.duration = dur;
            ev.is_short = is_short;
            xQueueSend(g_queue, &ev, portMAX_DELAY);
            
            if (is_short) {
                ledG->on();
                vTaskDelay(pdMS_TO_TICKS(300));
                ledG->off();
            } else {
                ledR->on();
                vTaskDelay(pdMS_TO_TICKS(300));
                ledR->off();
            }
            
            btn_pressed = false;
            ledY->off();
        }
        
        last = curr;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ========== TASK 2: Blink ==========
void vTaskBlink(void *pv) {
    PressEvent ev;
    for (;;) {
        if (xQueueReceive(g_queue, &ev, portMAX_DELAY)) {
            blinkY(ev.is_short ? 5 : 10);
            Serial.print("[Blink] ");
            Serial.print(ev.duration);
            Serial.println(ev.is_short ? "ms - SCURT" : "ms - LUNG");
        }
    }
}

// ========== TASK 3: Raport ==========
void vTaskReport(void *pv) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
        
        uint32_t t, s, l, sd, ld;
        float avg = 0;
        
        if (xSemaphoreTake(g_mutex, portMAX_DELAY)) {
            t = g_stats.total;
            s = g_stats.short_cnt;
            l = g_stats.long_cnt;
            sd = g_stats.short_dur;
            ld = g_stats.long_dur;
            
            if (t > 0) avg = (float)(sd + ld) / t;
            
            g_stats.total = 0;
            g_stats.short_cnt = 0;
            g_stats.long_cnt = 0;
            g_stats.short_dur = 0;
            g_stats.long_dur = 0;
            xSemaphoreGive(g_mutex);
        }
        
        Serial.println();
        Serial.println("=== RAPORT (10s) ===");
        Serial.print("Total: "); Serial.println(t);
        Serial.print("Scurte: "); Serial.println(s);
        Serial.print("Lungi: "); Serial.println(l);
        Serial.print("Medie: "); Serial.print(avg); Serial.println(" ms");
        Serial.println("====================");
        
        ledY->on();
        vTaskDelay(pdMS_TO_TICKS(200));
        ledY->off();
    }
}

// ========== SETUP ==========
void setup() {
    // Serial direct pentru debug
    Serial.begin(9600);
    delay(2000);  // Așteaptă USB
    
    Serial.println();
    Serial.println("=== LAB 3.2 - START ===");
    Serial.print("LED G: "); Serial.println(LED_GREEN_PIN);
    Serial.print("LED R: "); Serial.println(LED_RED_PIN);
    Serial.print("LED Y: "); Serial.println(LED_YELLOW_PIN);
    Serial.println("Buton: 1 (keypad row0,col0)");
    Serial.println("=======================");
    
    // LED-uri
    ledG = new Led(LED_GREEN_PIN);
    ledR = new Led(LED_RED_PIN);
    ledY = new Led(LED_YELLOW_PIN);
    ledG->begin();
    ledR->begin();
    ledY->begin();
    ledG->off();
    ledR->off();
    ledY->off();
    
    // Test
    Serial.print("Test LED... ");
    ledG->on(); delay(100); ledG->off();
    ledR->on(); delay(100); ledR->off();
    ledY->on(); delay(100); ledY->off();
    Serial.println("OK (stinse)");
    
    // Keypad pini
    for (int i = 0; i < 4; i++) {
        pinMode(ROW_PINS[i], OUTPUT);
        digitalWrite(ROW_PINS[i], HIGH);
        pinMode(COL_PINS[i], INPUT_PULLUP);
    }
    
    // Sincronizare
    g_mutex = xSemaphoreCreateMutex();
    g_queue = xQueueCreate(3, sizeof(PressEvent));
    
    Serial.println("Creare task-uri...");
    
    xTaskCreate(vTaskBtn, "Btn", 180, nullptr, 2, nullptr);
    xTaskCreate(vTaskBlink, "Blink", 180, nullptr, 1, nullptr);
    xTaskCreate(vTaskReport, "Report", 250, nullptr, 1, nullptr);
    
    Serial.println("=== SISTEM PORNIT ===");
    Serial.println("Apasa butonul 1...\n");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
