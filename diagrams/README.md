# Analysis Diagrams - ESP32 FreeRTOS Embedded System (lab2.2)

This folder contains UML analysis diagrams for the ESP32 FreeRTOS embedded system project.

## Diagrams Overview

| # | Diagram | Type | Description |
|---|---------|------|-------------|
| 1 | Class Diagram | Structural | All classes (Led, LcdI2c, Joystick, Keypad, Button, App, Mutex, BinarySemaphore, etc.) with attributes, methods, and relationships |
| 2 | Component Diagram | Structural | Module architecture showing FreeRTOS app, hardware abstraction, kernel primitives, and external libraries |
| 3 | Sequence - Joystick Press | Behavioral | Full interaction flow when joystick button is pressed/released, including semaphore signaling between tasks |
| 4 | Sequence - Code Entry | Behavioral | Keypad code entry, verification, access grant/deny, and programming mode flows |
| 5 | Activity - FreeRTOS Tasks | Behavioral | Parallel activity flows for vTaskDetect, vTaskDisplay, and vTaskLED |
| 6 | Activity - Code Verification | Behavioral | Detailed flowchart of App code entry logic including normal and programming modes |
| 7 | State Machine Diagram | Behavioral | System states: initialization, idle, pressing, result display, LED states, and App keypad states |
| 8 | Use Case Diagram | Behavioral | All user interactions: joystick control, keypad security, and system management |
| 9 | Deployment Diagram | Physical | Hardware architecture: ESP32, joystick, LEDs, LCD, keypad with GPIO pin mappings |
| 10 | Package/Module Diagram | Structural | Source code file organization and inter-module dependencies |
| 11 | Communication Diagram | Behavioral | Inter-task communication via semaphores, mutex, and shared data |
| 12 | Timing Diagram | Behavioral | Timeline of joystick press events showing short press vs long press behavior |

## File Structure

```
diagrams/
  puml/          -- PlantUML source files (.puml)
  png/           -- Rendered PNG images
  README.md      -- This file
```

## Regenerating Diagrams

To regenerate PNG images from PlantUML sources:

```bash
# Requires Java and Graphviz
java -jar plantuml.jar -tpng -o diagrams/png diagrams/puml/*.puml
```
