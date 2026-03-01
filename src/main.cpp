#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "serial_stdio/serial_stdio.h"
#include "led/led.h"
#include "lcd/lcd.h"
#include "joystick/joystick.h"

// ========== PINI ==========
static const uint8_t JOYSTICK_X_PIN = A0;
static const uint8_t JOYSTICK_Y_PIN = A1;
static const uint8_t JOYSTICK_SW_PIN = 3;
static const uint8_t LED_RED_PIN = 12;
static const uint8_t LED_GREEN_PIN = 11;
static const uint8_t LED_YELLOW_PIN = 13;

// ========== CONFIG ==========
#define NUM_TASKS 2

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
static TaskContext tasks[NUM_TASKS];
static uint8_t current_task = 0;
static volatile bool g_new_press_flag = false;  // Flag for Task 2

static Led *ledG = nullptr;
static Led *ledR = nullptr;
static Led *ledY = nullptr;
static LcdI2c *lcd = nullptr;
static Joystick *joystick = nullptr;

// Press counter tracking
static uint32_t total_presses = 0;

// ========== SCHEDULER BARE-METAL ==========

// Yield control to next task
void yield() {
    current_task = (current_task + 1) % NUM_TASKS;
}

void delay_ms(uint32_t ms) {
    tasks[current_task].state = STATE_BLOCKED;
    tasks[current_task].wait_until = millis() + ms;
    yield();
}


void blinkY(uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        ledY->on();
        delay(80);
        ledY->off();
        delay(80);
    }
    ledY->off();
}

// State machine: PC 0=wait for press, 1=button pressed (show duration on yellow), 2=button released (show result), 3=wait 5s
static void task1_detect(void *arg) {
    TaskContext *tc = &tasks[0];

    switch (tc->pc) {
        case 0:  // Wait for button press
        {
            joystick->scan();

            if (joystick->isPressed()) {
                tc->local_vars[0] = millis();  // Store press start time
                tc->pc = 1;
                ledR->off();
                ledG->off();
                // Yellow LED stays ON
                printf("State 0->1: Button pressed, Red/Green LEDs OFF, Yellow stays ON\n");
                return;
            }

            delay_ms(50);
            return;
        }

        case 1:  // Button is pressed - show duration on yellow LED intensity
        {
            joystick->scan();

            if (!joystick->isPressed()) {
                uint32_t duration = joystick->getPressDuration();
                tc->local_vars[1] = duration;  // Store duration
                tc->pc = 2;

                // Yellow LED stays ON (hardware verification)
                printf("Button released! Duration: %lu ms\n", duration);
                return;
            }

            // Yellow LED stays ON continuously - no blinking
            // Just add a small delay to avoid spamming
            delay_ms(50);

            return;
        }

        case 2:  // Show result (red/green LED and LCD display)
        {
            uint32_t duration = tc->local_vars[1];

            // Clear red and green LEDs (yellow stays ON)
            ledR->off();
            ledG->off();
            // Yellow LED stays ON for hardware verification
            printf("State 1->2: Turning Red/Green LEDs OFF, Yellow stays ON\n");

            // Determine color based on duration
            if (duration > 500) {
                ledG->on();  // Green for long press (>500ms)
                printf("Turning GREEN LED ON (duration %lu ms > 500), Yellow still ON\n", duration);
            } else {
                ledR->on();  // Red for short press (<=500ms)
                printf("Turning RED LED ON (duration %lu ms <= 500), Yellow still ON\n", duration);
            }

            // Update LCD with result
            lcd->clear();
            delay(10);
            lcd->setCursor(0, 0);
            
            if (duration > 500) {
                lcd->printf("Green LED:");
            } else {
                lcd->printf("Red LED:");
            }
            
            lcd->setCursor(0, 1);
            lcd->printf("%lu ms", duration);

            tc->local_vars[2] = millis();  // Store result start time
            tc->pc = 3;
            return;
        }

        case 3:  // Wait 5 seconds, then show "Press button" again
        {
            if (millis() - tc->local_vars[2] >= 5000) {
                ledR->off();
                ledG->off();
                // Yellow LED stays ON for hardware verification
                printf("State 3->0: 5 seconds elapsed, turning Red/Green LEDs OFF, Yellow stays ON\n");

                lcd->clear();
                delay(10);
                lcd->setCursor(0, 0);
                lcd->printf("Press Joystick");
                lcd->setCursor(0, 1);
                lcd->printf("Button");

                tc->pc = 0;
            } else {
                delay_ms(100);
            }
            return;
        }
    }
}

// State machine: Not used in this implementation
static void task2_blink(void *arg) {
    TaskContext *tc = &tasks[1];

    switch (tc->pc) {
        case 0:
        {
            delay_ms(100);
            return;
        }
    }
}



// Array of task function pointers
typedef void (*TaskFunc)(void*);
static TaskFunc task_funcs[NUM_TASKS] = {
    task1_detect,
    task2_blink
};

void init_tasks() {
    // Task 1 - Detect (prioritate 2)
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
}

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
void setup() {
    SerialStdio::begin(9600);
    delay(2000);

    printf("\n=== LAB 3.2 - JOYSTICK PRESS DURATION ===\n");
    printf("Joystick: X=A0, Y=A1, SW=D3\n");
    printf("LED R: %d, LED G: %d, LED Y: %d\n", LED_RED_PIN, LED_GREEN_PIN, LED_YELLOW_PIN);
    printf("LCD: I2C (0x27)\n");
    printf("Press joystick button - Yellow LED blinks during press\n");
    printf("Red LED: <=500ms, Green LED: >500ms\n");
    printf("=======================================\n");

    // LED-uri
    ledG = new Led(LED_GREEN_PIN);
    ledR = new Led(LED_RED_PIN);
    ledY = new Led(LED_YELLOW_PIN);
    ledG->begin();
    ledR->begin();
    ledY->begin();
    ledG->off();
    ledR->off();
    ledY->on();  // Keep yellow LED always ON for hardware verification
    printf("LEDs initialized - Yellow LED ON (pin 10), Red OFF (pin 8), Green OFF (pin 9)\n");

    // Joystick driver
    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();

    // Test LED
    printf("Test LED... ");
    ledG->on(); delay(100); ledG->off();
    ledR->on(); delay(100); ledR->off();
    ledY->on(); delay(100); ledY->off();
    printf("OK\n");

    // Fast blink test for yellow LED to verify pin 10
    printf("Testing yellow LED on pin 10 (blinking 5 times)...\n");
    for (int i = 0; i < 5; i++) {
        ledY->on();
        delay(200);
        ledY->off();
        delay(200);
    }
    printf("Yellow LED test complete\n");

    // Direct pin 10 test without LED class
    printf("Direct pin 10 test (3 blinks)...\n");
    pinMode(LED_YELLOW_PIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_YELLOW_PIN, HIGH);
        delay(300);
        digitalWrite(LED_YELLOW_PIN, LOW);
        delay(300);
    }
    printf("Direct pin 10 test complete\n");

    // LCD
    lcd = new LcdI2c(0x27, 16, 2);
    lcd->begin();
    lcd->setCursor(0, 0);
    lcd->printf("Press Joystick");
    lcd->setCursor(0, 1);
    lcd->printf("Button");

    // Inițializare tasks
    init_tasks();

    printf("=== SCHEDULER PORNIT ===\n");
    printf("Apasa pe joystick...\n\n");
}

void loop() {
    scheduler_run();
}
