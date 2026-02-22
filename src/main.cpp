/**
 * Laborator 3.2 - Monitorizare durată apăsare buton
 * Partea 1: Sistem de operare Non-Preemptive (bare-metal)
 * Implementare cu State Machines și Cooperative Scheduling
 * Buton: 1 de pe keypad
 * LED: Verde=3, Rosu=2, Galben=13
 */

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "serial_stdio/serial_stdio.h"
#include "led/led.h"
#include "lcd/lcd.h"
#include "button/button.h"

// ========== PINI ==========
static const uint8_t ROW_PINS[4] = {4, 5, 6, 7};
static const uint8_t COL_PINS[4] = {8, 9, 10, 11};
static const uint8_t LED_GREEN_PIN = 3;
static const uint8_t LED_RED_PIN = 2;
static const uint8_t LED_YELLOW_PIN = 13;

// ========== CONFIG ==========
#define SHORT_PRESS_MS 500
#define REPORT_PERIOD_MS 10000
#define NUM_TASKS 3

// ========== STRUCTURI CONTEXT ==========
typedef enum {
    STATE_READY,
    STATE_BLOCKED
} TaskState;

typedef struct TaskContext {
    TaskState state;
    uint32_t wait_until;      // Timestamp when task becomes ready
    uint16_t pc;              // Program counter for state machine
    uint8_t priority;
    const char *name;
    uint32_t local_vars[4];   // Local variables storage
} TaskContext;

// ========== VARIABILE GLOBALE ==========
typedef struct {
    uint32_t total;
    uint32_t short_cnt;
    uint32_t long_cnt;
    uint32_t short_dur;
    uint32_t long_dur;
} Stats;

static Stats g_stats;
static TaskContext tasks[NUM_TASKS];
static uint8_t current_task = 0;
static uint32_t last_report = 0;
static volatile bool g_new_press_flag = false;  // Flag for Task 2
static volatile bool g_last_press_was_short = true;  // Track last press type

static Led *ledG = nullptr;
static Led *ledR = nullptr;
static Led *ledY = nullptr;
static LcdI2c *lcd = nullptr;
static Button *button1 = nullptr;

// Button state tracking
static uint32_t total_presses = 0;

// ========== SCHEDULER BARE-METAL ==========

// Yield control to next task
void yield() {
    // Simple round-robin: move to next task
    current_task = (current_task + 1) % NUM_TASKS;
}

// Block current task for specified ms
void delay_ms(uint32_t ms) {
    tasks[current_task].state = STATE_BLOCKED;
    tasks[current_task].wait_until = millis() + ms;
    yield();
}

// ========== FUNCȚII HELPER ==========

void blinkY(uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        ledY->on();
        delay(80);
        ledY->off();
        delay(80);
    }
    ledY->off();
}

// ========== TASK 1: Detectare și măsurare durată apăsare ==========
// State machine: PC 0=check button, 1=wait release, 2=signal
static void task1_detect(void *arg) {
    TaskContext *tc = &tasks[0];
    
    switch (tc->pc) {
        case 0:  // Check button state
        {
            button1->scan();
            
            // Detectare eliberare (falling edge) - button was pressed and now released
            uint32_t dur = button1->getPressDuration();
            if (!button1->isPressed() && dur > 0) {
                bool is_short = (dur < SHORT_PRESS_MS);
                
                // Actualizare statistici
                g_stats.total++;
                if (is_short) {
                    g_stats.short_cnt++;
                    g_stats.short_dur += dur;
                } else {
                    g_stats.long_cnt++;
                    g_stats.long_dur += dur;
                }
                
                // Store duration for signaling
                tc->local_vars[0] = dur;
                tc->local_vars[1] = is_short ? 1 : 0;
                
                // Go to signal state
                tc->pc = 1;
                ledY->off();
                return;
            }
            
            // Detectare apăsare (rising edge)
            if (button1->isPressed()) {
                ledY->on();
            }
            
            delay_ms(50);
            return;
        }
        
        case 1:  // Signal visual (green/red LED)
        {
            uint32_t dur = tc->local_vars[0];
            bool is_short = tc->local_vars[1];
            
            printf("Press: %lu %s\n", dur, is_short ? "SHORT" : "LONG");
            
            // Increment press counter
            total_presses++;
            
            // Store the LED type and start time
            tc->local_vars[2] = is_short ? 1 : 0;  // 1 = green, 0 = red
            tc->local_vars[3] = millis();  // Start time
            
            // Turn on the LED and update LCD
            lcd->clear();
            delay(10);
            lcd->setCursor(0, 0);
            lcd->printf("Time: %lums", dur);
            lcd->setCursor(0, 1);
            lcd->print("                ");
            lcd->setCursor(0, 1);
            if (is_short) {
                ledG->on();
                lcd->printf("LED: GREEN");
            } else {
                ledR->on();
                lcd->printf("LED: RED");
            }
            
            tc->pc = 2;  // Go to wait state
            return;
        }
        
        case 2:  // Wait 5 seconds then turn off
        {
            uint32_t start_time = tc->local_vars[3];
            bool is_short = (tc->local_vars[2] == 1);
            
            if (millis() - start_time >= 5000) {
                // Time's up, turn off LED
                if (is_short) {
                    ledG->off();
                } else {
                    ledR->off();
                }
                
                // Show "Press button" message on LCD
                lcd->clear();
                delay(10);
                lcd->setCursor(0, 0);
                lcd->printf("Press Button");
                lcd->setCursor(0, 1);
                lcd->printf("to start");
                
                // Signal Task 2 to blink
                g_new_press_flag = true;
                g_last_press_was_short = is_short;
                
                tc->pc = 0;  // Back to checking
            } else {
                // Wait a bit and check again
                delay_ms(100);
            }
            return;
        }
    }
}

// ========== TASK 2: Contorizare și statistici cu blink ==========
// State machine: PC 0=check new press, 1=blink on, 2=blink off
static void task2_blink(void *arg) {
    TaskContext *tc = &tasks[1];
    
    switch (tc->pc) {
        case 0:  // Check if new press detected
        {
            if (g_new_press_flag) {
                g_new_press_flag = false;  // Clear flag
                
                // Get press type from global
                bool was_short = g_last_press_was_short;
                
                tc->local_vars[3] = was_short ? 5 : 10;  // Number of blinks
                tc->local_vars[4] = 0;  // Current blink count
                tc->pc = 1;
                return;
            }
            
            delay_ms(20);
            return;
        }
        
        case 1:  // Turn LED on
        {
            ledY->on();
            tc->pc = 2;
            delay_ms(80);
            return;
        }
        
        case 2:  // Turn LED off and check if more blinks needed
        {
            ledY->off();
            tc->local_vars[4]++;  // Increment blink count
            
            uint8_t max_blinks = tc->local_vars[3];
            if (tc->local_vars[4] < max_blinks) {
                tc->pc = 1;  // Next blink
                delay_ms(80);
            } else {
                tc->pc = 0;  // Done, go back to checking
            }
            return;
        }
    }
}

// ========== TASK 3: Raportare periodică ==========
// State machine: PC 0=check time, 1=report
static void task3_report(void *arg) {
    TaskContext *tc = &tasks[2];
    
    switch (tc->pc) {
        case 0:  // Check if report time
        {
            uint32_t elapsed = millis() - last_report;
            
            if (elapsed >= REPORT_PERIOD_MS) {
                tc->pc = 1;
                return;
            }
            
            delay_ms(100);
            return;
        }
        
        case 1:  // Generate report
        {
            uint32_t t = g_stats.total;
            uint32_t s = g_stats.short_cnt;
            uint32_t l = g_stats.long_cnt;
            uint32_t sd = g_stats.short_dur;
            uint32_t ld = g_stats.long_dur;
            float avg = (t > 0) ? ((float)(sd + ld) / t) : 0;
            
            printf("\n=== RAPORT (10s) ===\n");
            printf("Total apasari: %lu\n", t);
            printf("Apasari scurte: %lu\n", s);
            printf("Apasari lungi: %lu\n", l);
            printf("Durata medie: %.2f ms\n", (double)avg);
            printf("====================\n");
            
            // Resetare statistici
            g_stats.total = 0;
            g_stats.short_cnt = 0;
            g_stats.long_cnt = 0;
            g_stats.short_dur = 0;
            g_stats.long_dur = 0;
            
            // Semnal vizual raport
            ledY->on();
            delay_ms(200);
            ledY->off();
            
            last_report = millis();
            tc->pc = 0;
            return;
        }
    }
}

// Array of task function pointers
typedef void (*TaskFunc)(void*);
static TaskFunc task_funcs[NUM_TASKS] = {
    task1_detect,
    task2_blink,
    task3_report
};

// ========== INITIALIZARE TASKS ==========
void init_tasks() {
    // Task 1 - Detectare (prioritate 2)
    tasks[0].state = STATE_READY;
    tasks[0].priority = 2;
    tasks[0].name = "Detect";
    tasks[0].pc = 0;
    tasks[0].wait_until = 0;
    memset(tasks[0].local_vars, 0, sizeof(tasks[0].local_vars));
    
    // Task 2 - Blink (prioritate 1)
    tasks[1].state = STATE_READY;
    tasks[1].priority = 1;
    tasks[1].name = "Blink";
    tasks[1].pc = 0;
    tasks[1].wait_until = 0;
    memset(tasks[1].local_vars, 0, sizeof(tasks[1].local_vars));
    
    // Task 3 - Report (prioritate 1)
    tasks[2].state = STATE_READY;
    tasks[2].priority = 1;
    tasks[2].name = "Report";
    tasks[2].pc = 0;
    tasks[2].wait_until = 0;
    memset(tasks[2].local_vars, 0, sizeof(tasks[2].local_vars));
}

// ========== SCHEDULER MAIN LOOP ==========
void scheduler_run() {
    uint32_t now;
    
    while (1) {
        now = millis();
        
        // Check blocked tasks and unblock if time passed
        for (uint8_t i = 0; i < NUM_TASKS; i++) {
            if (tasks[i].state == STATE_BLOCKED) {
                if (now >= tasks[i].wait_until) {
                    tasks[i].state = STATE_READY;
                }
            }
        }
        
        // Execute current task if ready
        if (tasks[current_task].state == STATE_READY) {
            task_funcs[current_task](nullptr);
        } else {
            // Current task blocked, move to next
            current_task = (current_task + 1) % NUM_TASKS;
        }
    }
}

// ========== SETUP ==========
void setup() {
    SerialStdio::begin(9600);
    delay(2000);
    
    printf("\n=== LAB 3.2 - BARE-METAL ===\n");
    printf("Sistem NON-PREEMPTIVE cu scheduling cooperativ\n");
    printf("LED G: %d\n", LED_GREEN_PIN);
    printf("LED R: %d\n", LED_RED_PIN);
    printf("LED Y: %d\n", LED_YELLOW_PIN);
    printf("Buton: 1 (keypad row0,col0)\n");
    printf("============================\n");
    
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
    
    // Button driver
    button1 = new Button(ROW_PINS, COL_PINS, 4, 4, 0);
    button1->begin();
    
    // Test LED
    printf("Test LED... ");
    ledG->on(); delay(100); ledG->off();
    ledR->on(); delay(100); ledR->off();
    ledY->on(); delay(100); ledY->off();
    printf("OK\n");
    
    // LCD
    lcd = new LcdI2c(0x27, 16, 2);
    lcd->begin();
    lcd->setCursor(0, 0);
    lcd->printf("Press Button");
    lcd->setCursor(0, 1);
    lcd->printf("to start");
    
    // Keypad pini
    for (int i = 0; i < 4; i++) {
        pinMode(ROW_PINS[i], OUTPUT);
        digitalWrite(ROW_PINS[i], HIGH);
        pinMode(COL_PINS[i], INPUT_PULLUP);
    }
    
    // Reset statistici
    memset(&g_stats, 0, sizeof(Stats));
    last_report = millis();
    total_presses = 0;
    
    // Inițializare tasks
    init_tasks();
    
    printf("=== SCHEDULER PORNIT ===\n");
    printf("Apasa butonul 1...\n\n");
}

void loop() {
    // Nu se folosește loop în bare-metal
    // Toate task-urile rulează în scheduler_run()
    scheduler_run();
}
