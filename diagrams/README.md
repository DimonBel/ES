# Analysis Diagrams - ESP32 FreeRTOS Embedded System (lab2.2)

This folder contains UML analysis diagrams for the ESP32 FreeRTOS embedded system project.
Both full (combined) and split (smaller, easier to read) versions are provided.

## Full Diagrams (Combined)

| # | Diagram | Type | Description |
|---|---------|------|-------------|
| 1 | Class Diagram | Structural | All classes with attributes, methods, and relationships |
| 2 | Component Diagram | Structural | Full module architecture |
| 3 | Sequence - Joystick Press | Behavioral | Full joystick interaction flow |
| 4 | Sequence - Code Entry | Behavioral | Full keypad code entry flow |
| 5 | Activity - FreeRTOS Tasks | Behavioral | All 3 task activity flows |
| 6 | Activity - Code Verification | Behavioral | Full App code entry flowchart |
| 7 | State Machine Diagram | Behavioral | All system states combined |
| 8 | Use Case Diagram | Behavioral | All user interactions |
| 9 | Deployment Diagram | Physical | Hardware architecture with pin mappings |
| 10 | Package/Module Diagram | Structural | Full file organization |
| 11 | Communication Diagram | Behavioral | Inter-task communication |
| 12 | Timing Diagram | Behavioral | Joystick press event timeline |

## Split Diagrams (Smaller, Easier to Read)

### Class Diagram (split into 3)

| File | Description |
|------|-------------|
| 01a - Hardware Drivers | Led, LcdI2c, Joystick, Keypad, Button classes |
| 01b - Kernel Primitives | Mutex, BinarySemaphore, SharedData |
| 01c - Application Classes | App, CommandParser, I2CScanner, SerialStdio |

### Component Diagram (split into 2)

| File | Description |
|------|-------------|
| 02a - FreeRTOS Application | Entry point, freertos_app module internals |
| 02b - Hardware & Kernel | Hardware drivers, kernel primitives, external libs |

### Sequence - Joystick (split into 2)

| File | Description |
|------|-------------|
| 03a - Press Detection | vTaskDetect polling, press/release detection, signaling |
| 03b - Display & LED Response | vTaskDisplay and vTaskLED response to events |

### Sequence - Code Entry (split into 2)

| File | Description |
|------|-------------|
| 04a - Normal Code Entry | Digit entry, verification, access granted/denied |
| 04b - Programming Mode | Enter prog mode, set new password, cancel |

### Activity - FreeRTOS Tasks (split into 3)

| File | Description |
|------|-------------|
| 05a - vTaskDetect | Joystick polling and event detection logic |
| 05b - vTaskDisplay | LCD update logic with mutex protection |
| 05c - vTaskLED | LED control logic based on press duration |

### Activity - Code Verification (split into 2)

| File | Description |
|------|-------------|
| 06a - Normal Mode | Code entry, verification, access grant/deny flow |
| 06b - Programming Mode | New password entry and confirmation flow |

### State Machine (split into 3)

| File | Description |
|------|-------------|
| 07a - FreeRTOS System | Init, Idle, Pressing, Result states |
| 07b - LED States | All OFF, Yellow, Green, Red transitions |
| 07c - App Keypad | Welcome, Entry, Granted, Denied, Programming states |

### Package/Module (split into 2)

| File | Description |
|------|-------------|
| 10a - FreeRTOS App Module | freertos_app internal module dependencies |
| 10b - Drivers & Kernel | Hardware drivers, kernel primitives, other modules |

## Small Architecture Diagrams

| # | Diagram | Description |
|---|---------|-------------|
| 13 | High-Level Architecture | Simplified overview: User, ESP32 system, hardware |
| 14 | Software Layer Architecture | Layered view: App, Abstraction, Kernel, Platform, HW |
| 15 | FreeRTOS Task Architecture | Task priorities, semaphores, SharedData flow |
| 16 | Hardware Connection Architecture | GPIO/I2C/UART pin connections |
| 17 | Data Flow Architecture | Input-to-output data flow |

## File Structure

```
diagrams/
  puml/          -- PlantUML source files (.puml)
  png/           -- Rendered PNG images
  README.md      -- This file
```

## Regenerating Diagrams

```bash
# Requires Java and Graphviz
java -jar plantuml.jar -tpng -o diagrams/png diagrams/puml/*.puml
```
