# Lab 3.2: Button Press Duration Monitoring System - Complete Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Hardware Configuration](#hardware-configuration)
4. [Module Structure](#module-structure)
5. [Key Components](#key-components)
6. [Software Design](#software-design)
7. [Data Flow](#data-flow)
8. [Workflow Diagrams](#workflow-diagrams)

---

## Overview

This project implements a **Non-Preemptive Bare-Metal Operating System** with cooperative scheduling for monitoring button press durations. The system features:

- **Button Press Detection**: Monitors button press duration on a 4x4 keypad
- **LED Feedback**: Visual indication with Green (short press) and Red (long press) LEDs
- **LCD Display**: Real-time feedback showing press duration and LED status
- **Statistical Reporting**: Periodic reports every 10 seconds with press statistics
- **State Machine Architecture**: Each task implemented as a finite state machine

---

## Architecture

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Bare-Metal OS Scheduler                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  Task 1:     │  │  Task 2:     │  │  Task 3:     │          │
│  │  Button      │  │  Blink       │  │  Report      │          │
│  │  Detection   │  │  Feedback    │  │  Statistics  │          │
│  │  (Priority 2)│  │  (Priority 1)│  │  (Priority 1)│          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
│         │                 │                 │                    │
│         └─────────────────┴─────────────────┘                    │
│                           │                                      │
│                           ▼                                      │
│                ┌──────────────────┐                             │
│                │  Round-Robin     │                             │
│                │  Cooperative     │                             │
│                │  Scheduler       │                             │
│                └──────────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│   Button      │  │   LED         │  │   LCD        │
│   Driver      │  │   Drivers     │  │   Driver     │
└───────────────┘  └───────────────┘  └───────────────┘
```

---

## Hardware Configuration

### Pin Assignments

| Component | Pin | Description |
|-----------|-----|-------------|
| **LED Green** | 3 | Indicates short press (<500ms) |
| **LED Red** | 2 | Indicates long press (≥500ms) |
| **LED Yellow** | 13 | Indicates button is pressed |
| **Keypad Rows** | 4, 5, 6, 7 | Matrix row outputs |
| **Keypad Cols** | 8, 9, 10, 11 | Matrix column inputs |
| **LCD I2C** | A4/A5 | I2C communication (address 0x27) |

### Keypad Matrix Layout

```
        Col0   Col1   Col2   Col3
Row0:    1      2      3      A
Row1:    4      5      6      B
Row2:    7      8      9      C
Row3:    *      0      #      D
```

**Button 1** is located at Row0, Col0

---

## Module Structure

```
D:\ES\src\modules\
├── button\              # Button matrix driver
│   ├── button.h
│   └── button.cpp
├── lcd\                 # LCD I2C driver
│   ├── lcd.h
│   └── lcd.cpp
├── led\                 # LED driver
│   ├── led.h
│   └── led.cpp
├── serial_stdio\        # Serial stdio redirection
│   ├── serial_stdio.h
│   └── serial_stdio.cpp
├── app\                 # Application (not used in Lab 3.2)
│   ├── app.h
│   └── app.cpp
├── command\             # Command parser (not used)
│   ├── command.h
│   └── command.cpp
├── keypad\              # Keypad driver (not used in Lab 3.2)
│   ├── keypad.h
│   └── keypad.cpp
├── stdio_redirect\      # Stdio redirect utilities
│   ├── stdio_redirect.h
│   └── stdio_redirect.cpp
└── utils\               # Utility functions
    └── i2c_scanner.h
```

---

## Key Components

### 1. Button Driver (`button/`)

**Purpose**: Low-level hardware abstraction for 4x4 keypad matrix scanning

**Key Features**:
- Matrix scanning algorithm
- Edge detection (rising/falling)
- Debouncing built-in
- Press duration measurement

**API**:
```cpp
Button::begin()          // Initialize pins
Button::scan()           // Scan matrix and update state
Button::isPressed()      // Check if button is currently pressed
Button::getPressDuration() // Get press duration (ms), resets after read
```

**State Machine**:
```
IDLE → PRESSED → RELEASED
  ↑         ↓
  └─────────┘
```

### 2. LCD Driver (`lcd/`)

**Purpose**: I2C LCD display with printf support

**Key Features**:
- 16x2 character display
- I2C communication (address 0x27)
- Printf formatting support
- Cursor positioning

**API**:
```cpp
LcdI2c::begin()          // Initialize LCD
LcdI2c::clear()          // Clear display
LcdI2c::setCursor(col, row) // Set cursor position
LcdI2c::printf(format, ...) // Formatted output
LcdI2c::print(text)      // Print text
```

### 3. LED Driver (`led/`)

**Purpose**: Simple LED control with state tracking

**API**:
```cpp
Led::on()                // Turn LED on
Led::off()               // Turn LED off
Led::toggle()            // Toggle LED state
Led::state()             // Get current state
```

### 4. Serial Stdio (`serial_stdio/`)

**Purpose**: Redirect printf/scanf to Serial

**Key Features**:
- Standard C I/O redirection
- `\n` → `\r\n` conversion
- Character echo for backspace

**Usage**:
```cpp
SerialStdio::begin(9600);
printf("Hello, World!\n");  // Output to Serial
```

---

## Software Design

### Cooperative Scheduler

**Type**: Non-preemptive, round-robin scheduling

**Task States**:
```
┌─────────────┐
│   READY     │ ◄─────────────────┐
└──────┬──────┘                   │
       │                          │
       │ yield() / delay_ms()     │
       ▼                          │
┌─────────────┐                   │
│   BLOCKED   │ ──── timeout ────┘
└─────────────┘
```

**Scheduler Algorithm**:
```cpp
while (1) {
    // 1. Check blocked tasks
    for (each task) {
        if (task.state == BLOCKED && now >= task.wait_until)
            task.state = READY;
    }
    
    // 2. Execute current task if ready
    if (tasks[current_task].state == READY) {
        task_funcs[current_task](nullptr);
    } else {
        current_task = (current_task + 1) % NUM_TASKS;
    }
}
```

### Task 1: Button Detection

**Purpose**: Detect button presses, measure duration, update statistics

**State Machine**:
```
┌──────────┐
│   PC=0   │ ← Check button state
└────┬─────┘
     │
     ├─ Button Released & Duration > 0
     │  → Store duration, update stats
     │  → Go to PC=1
     │
     ├─ Button Pressed
     │  → Turn on Yellow LED
     │
     └─ No change
        → delay_ms(50)
```

**Detailed States**:
- **PC=0**: Scan button, detect release edge
- **PC=1**: Update LCD, turn on Green/Red LED
- **PC=2**: Wait 5 seconds, turn off LED, return to PC=0

### Task 2: Blink Feedback

**Purpose**: Provide visual feedback after button press

**State Machine**:
```
┌──────────┐
│   PC=0   │ ← Wait for new press flag
└────┬─────┘
     │
     ├─ Flag Set
     │  → Set blink count (5 or 10)
     │  → Go to PC=1
     │
     └─ No flag
        → delay_ms(20)

┌──────────┐
│   PC=1   │ ← Turn Yellow LED on
└────┬─────┘
     │
     └─ → Go to PC=2

┌──────────┐
│   PC=2   │ ← Turn LED off, check count
└────┬─────┘
     │
     ├─ Count < Max
     │  → Increment count, go to PC=1
     │
     └─ Count >= Max
        → Reset, go to PC=0
```

### Task 3: Report Statistics

**Purpose**: Generate periodic statistical reports

**State Machine**:
```
┌──────────┐
│   PC=0   │ ← Check if 10 seconds elapsed
└────┬─────┘
     │
     ├─ Time elapsed
     │  → Go to PC=1
     │
     └─ Not elapsed
        → delay_ms(100)

┌──────────┐
│   PC=1   │ ← Generate report
└────┬─────┘
     │
     ├─ Print statistics
     ├─ Reset counters
     ├─ Flash Yellow LED
     └─ Go to PC=0
```

---

## Data Flow

### Button Press Flow

```
User presses Button 1
        ↓
Button::scan() detects rising edge
        ↓
Button stores press start time
        ↓
Yellow LED turns on
        ↓
User releases Button 1
        ↓
Button::scan() detects falling edge
        ↓
Button calculates duration
        ↓
Task 1 (PC=0) processes release
        ↓
Updates global statistics
        ↓
Stores duration in local_vars[0]
        ↓
Transitions to PC=1
        ↓
Updates LCD: "Time: XXXms"
        ↓
Turns on Green/Red LED
        ↓
Transitions to PC=2
        ↓
Waits 5 seconds
        ↓
Turns off Green/Red LED
        ↓
Sets g_new_press_flag = true
        ↓
Task 2 detects flag
        ↓
Blinks Yellow LED (5 or 10 times)
        ↓
Resets to PC=0
```

### Statistical Reporting Flow

```
Every 10 seconds
        ↓
Task 3 (PC=0) checks time
        ↓
Transitions to PC=1
        ↓
Reads global statistics
        ↓
Calculates average duration
        ↓
Prints report to Serial
        ↓
Resets statistics counters
        ↓
Flashes Yellow LED
        ↓
Returns to PC=0
```

---

## Workflow Diagrams

### Overall System Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                      SYSTEM STARTUP                         │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
        ┌──────────────────┐
        │ Initialize Serial│
        │    (9600 baud)   │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Initialize LED  │
        │    Drivers       │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Initialize LCD  │
        │  Display Welcome │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Initialize      │
        │  Button Driver   │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Initialize      │
        │  Task Contexts   │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Start Scheduler │
        │     (Loop)       │
        └──────────────────┘
                   │
        ┌──────────┼──────────┐
        │          │          │
        ▼          ▼          ▼
   ┌──────┐  ┌──────┐  ┌──────┐
   │Task1 │  │Task2 │  │Task3 │
   └──────┘  └──────┘  └──────┘
```

### Button Press Processing Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                    BUTTON PRESSED                           │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
        ┌──────────────────┐
        │  Yellow LED ON   │
        │ (Button pressed) │
        └────────┬─────────┘
                 │
        ◄────────┴────────►
        │                 │
        │                 │
        ▼                 ▼
  ┌──────────┐      ┌──────────┐
  │ Duration │      │ Short?   │
  │ < 500ms  │      │ Long?    │
  └────┬─────┘      └────┬─────┘
       │                 │
       │ YES             │ NO
       ▼                 ▼
┌──────────┐      ┌──────────┐
│ GREEN    │      │ RED      │
│ LED ON   │      │ LED ON   │
└────┬─────┘      └────┬─────┘
     │                 │
     │ 5 seconds       │ 5 seconds
     │                 │
     ▼                 ▼
┌──────────┐      ┌──────────┐
│ YELLOW   │      │ YELLOW   │
│ BLINK 5x │      │ BLINK    │
└──────────┘      │ 10x      │
                  └──────────┘
```

### Task Scheduling Flow

```
┌─────────────────────────────────────────────────────────────┐
│                  SCHEDULER MAIN LOOP                         │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
        ┌──────────────────┐
        │  Get Current     │
        │     Time         │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Check Blocked   │
        │     Tasks        │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Unblock Ready   │
        │     Tasks        │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Execute Current │
        │     Task         │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │  Yield to Next   │
        │     Task         │
        └────────┬─────────┘
                 │
                 └────────► (Repeat)
```

---

## Key Technical Details

### State Management

Each task maintains its state in a `TaskContext` structure:

```cpp
typedef struct TaskContext {
    TaskState state;           // READY or BLOCKED
    uint32_t wait_until;       // Wake-up time for blocked tasks
    uint16_t pc;               // Program counter (state machine state)
    uint8_t priority;          // Task priority
    const char *name;          // Task name
    uint32_t local_vars[4];    // Local variables storage
} TaskContext;
```

### Inter-Task Communication

Tasks communicate through **global flags**:

```cpp
volatile bool g_new_press_flag;         // Signals Task 2 to blink
volatile bool g_last_press_was_short;   // Indicates press type
```

### Statistics Tracking

Global statistics structure:

```cpp
typedef struct {
    uint32_t total;        // Total button presses
    uint32_t short_cnt;    // Short press count
    uint32_t long_cnt;     // Long press count
    uint32_t short_dur;    // Total short press duration
    uint32_t long_dur;     // Total long press duration
} Stats;
```

---

## Output Examples

### Serial Output

```
=== LAB 3.2 - BARE-METAL ===
Sistem NON-PREEMPTIVE cu scheduling cooperativ
LED G: 3
LED R: 2
LED Y: 13
Buton: 1 (keypad row0,col0)
============================
Test LED... OK
=== SCHEDULER PORNIT ===
Apasa butonul 1...

Press: 150 SHORT
Press: 1951 LONG

=== RAPORT (10s) ===
Total apasari: 2
Apasari scurte: 1
Apasari lungi: 1
Durata medie: 1050.50 ms
====================
```

### LCD Display States

**State 1: Idle**
```
┌────────────────────┐
│ Press Button       │
│ to start           │
└────────────────────┘
```

**State 2: Short Press**
```
┌────────────────────┐
│ Time: 150ms        │
│ LED: GREEN         │
└────────────────────┘
```

**State 3: Long Press**
```
┌────────────────────┐
│ Time: 1951ms       │
│ LED: RED           │
└────────────────────┘
```

---

## Design Principles

1. **Separation of Concerns**: Hardware drivers separated from application logic
2. **Encapsulation**: Each module provides clean API
3. **State Machines**: Tasks implemented as finite state machines
4. **Cooperative Multitasking**: Non-preemptive, explicit yielding
5. **Standard I/O**: Uses printf/scanf for all communication
6. **Modularity**: Easy to extend and modify

---

## Conclusion

This system demonstrates a complete bare-metal operating system implementation with:
- Cooperative scheduling
- State machine-based tasks
- Hardware abstraction layers
- Inter-task communication
- Statistical data collection

The architecture is clean, maintainable, and follows embedded systems best practices.