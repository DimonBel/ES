# Simplified Sequence Diagrams - Actuator Control System

## Overview

This document contains simplified sequence diagrams, each focusing on a specific interaction in the system.

## 1. Button Control Flow

```mermaid
sequenceDiagram
    participant User as User
    participant Btn as Button (GPIO 18)
    participant Task as ActuatorControl Task
    participant Data as SharedData
    participant HW as Actuator (GPIO 23)

    User->>Btn: Press button
    Btn->>Task: Trigger interrupt
    Task->>Task: Edge detection
    Task->>Task: Check cooldown (250ms)
    Task->>Data: Toggle actuator_command
    Data->>Data: Update timestamp
    Task->>HW: Update actuator state (150ms later)
    HW-->>User: Actuator response
```

## 2. Serial Command Processing

```mermaid
sequenceDiagram
    participant User as User
    participant Serial as Serial Port (115200)
    participant Task as ActuatorControl Task
    participant Data as SharedData

    User->>Serial: Send "on"
    Serial->>Task: Receive characters
    Task->>Task: Parse command
    Task->>Data: actuator_command = true
    Data-->>Task: Confirmed
    Task-->>User: Command accepted
```

## 3. Signal Conditioning Process

```mermaid
sequenceDiagram
    participant Task as SignalConditioning Task
    participant Data as SharedData
    participant Filter as Signal Conditioner
    participant HW as Actuator + LED

    Task->>Data: Read actuator_command
    Data-->>Task: Return command
    Task->>Filter: conditionSignal(cmd, 50, 100)
    Filter->>Filter: Debounce (50ms)
    Filter->>Filter: Validation (100ms)
    Filter-->>Task: Return conditioned signal
    Task->>HW: Update hardware state
    HW-->>Task: Complete
```

## 4. Display Update Flow

```mermaid
sequenceDiagram
    participant Task as Display Task
    participant Sem as Semaphore
    participant Data as SharedData
    participant Mutex as Mutex
    participant LCD as LCD Display

    loop Every 500ms
        Task->>Sem: Wait for signal (100ms timeout)
        Task->>Data: Read state and count
        Data-->>Task: Return data
        Task->>Mutex: Acquire lock (100ms timeout)
        Mutex-->>Task: Success
        Task->>LCD: Update display
        LCD-->>Task: Complete
        Task->>Mutex: Release lock
    end
```

## 5. Multi-Input Coordination

```mermaid
sequenceDiagram
    participant Btn as Button
    participant JS as Joystick Button
    participant Serial as Serial Command
    participant Task as ActuatorControl Task
    participant Data as SharedData

    par Three input methods
        Btn->>Task: Button press
        and
        JS->>Task: Joystick press
        and
        Serial->>Task: "toggle" command
    end

    Task->>Task: Unified processing
    Task->>Data: Update actuator_command
    Data-->>Task: Confirmed
    Task->>Task: Trigger signal conditioning
```

## 6. Hardware Control Details

```mermaid
sequenceDiagram
    participant Task as SignalConditioning Task
    participant Act as Actuator
    participant LED as LED Indicator
    participant GPIO23 as GPIO 23
    participant GPIO26 as GPIO 26

    Task->>Act: turnOn()
    Act->>GPIO23: digitalWrite(HIGH)
    GPIO23-->>Act: Complete
    Act-->>Task: Return

    Task->>LED: on()
    LED->>GPIO26: digitalWrite(HIGH)
    GPIO26-->>LED: Complete
    LED-->>Task: Return

    Note over Task: Increment counter
```

## 7. I2C LCD Communication

```mermaid
sequenceDiagram
    participant Task as Display Task
    participant Mutex as Mutex
    participant I2C as I2C Bus
    participant LCD as LCD Controller

    Task->>Mutex: take(100ms)
    Mutex-->>Task: Success
    Task->>I2C: Write clear command (0x01)
    I2C->>LCD: I2C transfer
    LCD-->>I2C: ACK
    I2C-->>Task: Complete
    Task->>I2C: Write cursor position
    I2C->>LCD: I2C transfer
    LCD-->>I2C: ACK
    I2C-->>Task: Complete
    Task->>I2C: Write text
    I2C->>LCD: Character transfer
    LCD-->>I2C: ACK
    I2C-->>Task: Complete
    Task->>Mutex: give()
    Mutex-->>Task: Released
```

## 8. Double-Click Protection

```mermaid
sequenceDiagram
    participant User as User
    participant Task as ActuatorControl Task
    participant Timer as Cooldown Timer

    User->>Task: First press
    Task->>Timer: Start timer (250ms)
    Task->>Task: Execute toggle
    Task-->>User: Response

    User->>Task: Second press (100ms later)
    Task->>Timer: Check time
    Timer-->>Task: Not expired (100ms < 250ms)
    Task->>Task: Ignore this press
    Task-->>User: "Double-click prevented"

    Note over User,Timer: Must wait 250ms before next trigger
```

## 9. State Change Detection

```mermaid
sequenceDiagram
    participant Task as SignalConditioning Task
    participant Data as SharedData
    participant Filter as Signal Conditioner
    participant HW as Hardware

    loop Every 50ms
        Task->>Data: Read actuator_command
        Data-->>Task: Command value

        Task->>Filter: Execute signal conditioning
        Filter-->>Task: Conditioned value

        alt Signal changed
            Task->>HW: Update actuator
            HW-->>Task: Complete
            Task->>HW: Update LED
            HW-->>Task: Complete
            Task->>Data: actuator_toggle_count++
        else Signal unchanged
            Task->>Task: Maintain current state
        end
    end
```

## 10. Task Scheduling

```mermaid
sequenceDiagram
    participant RTOS as FreeRTOS Scheduler
    participant Task1 as ActuatorControl (P3)
    participant Task2 as SignalConditioning (P3)
    participant Task3 as Display (P2)

    loop System running
        RTOS->>Task1: Wake (50ms period)
        Task1->>Task1: Process inputs
        Task1->>RTOS: Block

        RTOS->>Task2: Wake (50ms period)
        Task2->>Task2: Signal conditioning
        Task2->>RTOS: Block

        RTOS->>Task3: Wake (500ms period)
        Task3->>Task3: Update display
        Task3->>RTOS: Block
    end

    Note over RTOS: P3 priority higher than P2
```