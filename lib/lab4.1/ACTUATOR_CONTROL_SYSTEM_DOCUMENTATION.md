# Actuator Control System Implementation Documentation

## Overview

This document provides a comprehensive explanation of the Actuator Control System implementation for Lab 4.1. The system uses FreeRTOS on ESP32 to control a binary actuator (relay) with signal conditioning, multiple input methods (button, joystick, serial), and visual feedback via LED and LCD display.

## System Architecture

### Hardware Components

| Component | GPIO Pin | Type | Function |
|-----------|----------|------|----------|
| Actuator | 23 | Digital Output | Relay control (binary ON/OFF) |
| Button | 18 | Digital Input | Toggle actuator (internal pull-up) |
| Joystick X | 34 | Analog Input | Horizontal position (0-4095) |
| Joystick Y | 35 | Analog Input | Vertical position (0-4095) |
| Joystick SW | 25 | Digital Input | Joystick button (toggle actuator) |
| LED | 26 | Digital Output | Visual indicator (mirrors actuator state) |
| LCD SDA | 21 | I2C Data | Display communication |
| LCD SCL | 22 | I2C Clock | Display communication |
| Serial | TX0/RX0 | UART | Debug output & command input (115200 baud) |

### Signal Conditioning Configuration

- **Debounce Time**: 50ms
- **Validation Time**: 100ms
- **Button Cooldown**: 250ms (prevents double-click)
- **Acquisition Rate**: 50ms (20Hz)

## Software Architecture

### Module Structure

```
src/modules/
├── freertos_app/          # FreeRTOS application framework
│   ├── init/             # Hardware initialization & task creation
│   ├── state/            # Shared data & configuration
│   ├── sync/             # Synchronization primitives
│   └── tasks/            # FreeRTOS tasks implementation
├── actuator/             # Actuator (relay) driver
├── joystick/             # Joystick driver
├── led/                  # LED control module
├── lcd/                  # I2C LCD display module
├── signal_conditioner/   # Signal conditioning module
└── kernel_primitives/    # FreeRTOS wrappers
    ├── task/
    ├── mutex/
    └── semaphore/
```

### FreeRTOS Tasks

The system uses three FreeRTOS tasks with different priorities:

| Task | Priority | Stack Size | Frequency | Purpose |
|------|----------|------------|-----------|---------|
| vTaskActuatorControl | 3 (highest) | 4096 bytes | 50ms | Input processing & command generation |
| vTaskSignalConditioning | 3 (highest) | 4096 bytes | 50ms | Signal conditioning & actuator control |
| vTaskDisplay | 2 (medium) | 4096 bytes | 500ms | LCD display updates |

## Detailed Implementation

### 1. Actuator Module (`actuator/`)

**File**: `actuator.h`, `actuator.cpp`

The Actuator class provides an interface for controlling a relay actuator.

#### Public API

```cpp
class Actuator {
public:
    enum State {
        STATE_OFF = 0,
        STATE_ON = 1,
        STATE_ERROR = 2
    };

    // Constructor
    explicit Actuator(uint8_t pin);

    // Initialization
    void begin();

    // Control methods
    void turnOn();
    void turnOff();
    void toggle();

    // State query
    State getState() const;
    const char* getStateString() const;
};
```

#### Key Features

1. **Binary Control**: Turns relay ON (HIGH) or OFF (LOW)
2. **State Tracking**: Maintains current state (ON/OFF/ERROR)
3. **Toggle Function**: Switches current state
4. **String Representation**: Returns human-readable state

### 2. Signal Conditioner Module (`signal_conditioner/`)

**File**: `signal_conditioner.h`, `signal_conditioner.cpp`

The SignalConditioner class implements robust signal processing with three stages: saturation, debouncing, and validation.

#### Public API

```cpp
class SignalConditioner {
public:
    // Saturation (clamping) - not used for binary signals
    static bool saturateSignal(int rawValue, int minValue, int maxValue, int *saturatedValue);

    // Debouncing - filters rapid changes
    bool debounceSignal(bool rawSignal, uint32_t debounceTimeMs);

    // Persistent state validation - confirms stability
    bool validatePersistentState(bool targetState, uint32_t validationTimeMs);

    // Combined method - applies both debouncing and validation
    bool conditionSignal(bool rawSignal, uint32_t debounceTimeMs, uint32_t validationTimeMs);
};
```

#### Key Features

1. **Saturation**: Clamps analog values to a specified range (not used for binary signals)
2. **Debouncing**: Filters rapid changes by requiring signal stability for debounce time
3. **Validation**: Confirms state persistence for validation time before accepting
4. **Combined Processing**: Applies both debouncing and validation in sequence

#### Debouncing Implementation

```cpp
bool SignalConditioner::debounceSignal(bool rawSignal, uint32_t debounceTimeMs) {
    uint32_t currentTime = xTaskGetTickCount();

    if (rawSignal != _debounceState) {
        // Signal changed - start debounce timer
        _debounceStartTime = currentTime;
        _debounceState = rawSignal;
        return _conditionedSignal;  // Return previous state during debounce
    }

    // Check if debounce time elapsed
    if ((currentTime - _debounceStartTime) >= pdMS_TO_TICKS(debounceTimeMs)) {
        _conditionedSignal = rawSignal;
    }

    return _conditionedSignal;
}
```

#### Validation Implementation

```cpp
bool SignalConditioner::validatePersistentState(bool targetState, uint32_t validationTimeMs) {
    uint32_t currentTime = xTaskGetTickCount();

    if (targetState != _validationTarget) {
        // Target changed - reset validation
        _validationTarget = targetState;
        _validationStartTime = currentTime;
        return _validatedState;  // Return previous state during validation
    }

    // Check if validation time elapsed
    if ((currentTime - _validationStartTime) >= pdMS_TO_TICKS(validationTimeMs)) {
        _validatedState = targetState;
    }

    return _validatedState;
}
```

#### Why Two-Stage Conditioning?

1. **Debouncing (50ms)**: Eliminates electrical noise and mechanical bounce
2. **Validation (100ms)**: Ensures the signal is stable before changing actuator state

This provides robust protection against:
- Electrical noise
- Mechanical button bounce
- Signal interference
- Spurious commands

### 3. Joystick Module (`joystick/`)

**File**: `joystick.h`, `joystick.cpp`

The Joystick class provides an interface for reading joystick position and button state.

#### Public API

```cpp
class Joystick {
public:
    enum Direction {
        DIR_CENTER = 0,
        DIR_UP,
        DIR_DOWN,
        DIR_LEFT,
        DIR_RIGHT
    };

    // Constructor
    Joystick(uint8_t pinX, uint8_t pinY, uint8_t pinSW);

    // Initialization
    void begin();

    // Reading methods
    int readX();           // Returns 0-4095
    int readY();           // Returns 0-4095
    bool isPressed();      // Returns true if button pressed
    Direction getDirection();  // Returns current direction

    // Press duration measurement
    bool isPressDetected();    // Edge detection for button
    uint32_t getPressDuration();  // Returns press duration in ms
};
```

### 4. FreeRTOS Application Framework (`freertos_app/`)

#### 4.1 Shared Data Structure (`state/state.h`)

```cpp
struct SharedData {
    bool actuator_command;              // Command from user (ON/OFF)
    bool actuator_state;                // Current actuator state
    bool actuator_conditioned;          // Conditioned signal state
    uint32_t actuator_command_time;     // Last command timestamp
    uint32_t actuator_toggle_count;     // Total toggle count
    bool serial_command_received;       // Serial command flag
    char serial_command_buffer[16];     // Serial command buffer
    uint8_t serial_command_index;       // Current buffer index
};
```

This structure is shared between all tasks and provides thread-safe access to system state.

#### 4.2 Synchronization Primitives (`sync/sync.cpp`)

```cpp
// Protects LCD access from concurrent tasks
kernel_primitives::Mutex lcdMutex;

// Signals display task to update LCD
kernel_primitives::BinarySemaphore semActuatorDisplay;
```

#### 4.3 Task Implementation (`tasks/tasks.cpp`)

##### vTaskActuatorControl - Input Processing

**Purpose**: Process all inputs (button, joystick, serial) and generate actuator commands.

**Frequency**: 50ms (20Hz)

**Algorithm**:

```
1. Read button state (GPIO 18) with edge detection
2. Check button cooldown (250ms) to prevent double-click
3. Read joystick button state
4. Check for serial commands ('on', 'off', 'toggle', 'status')
5. Update actuator_command in SharedData
6. Signal display task via semaphore
```

**Button Toggle Logic**:

```cpp
// Edge detection with cooldown
if (buttonPressed && !lastButtonPressed) {
    if ((currentTime - lastButtonToggleTime) > 250) {
        kernel_primitives::delayMs(50);  // Simple debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            sharedData.actuator_command = !sharedData.actuator_command;
            sharedData.actuator_command_time = currentTime;
            lastButtonToggleTime = currentTime;
            printf("[ACTUATOR_CTRL] Button toggled to: %s\n",
                   sharedData.actuator_command ? "ON" : "OFF");
        }
    } else {
        printf("[ACTUATOR_CTRL] Double-click prevented\n");
    }
}
```

**Serial Command Processing** (in main.cpp loop):

```cpp
if (Serial.available()) {
    char c = Serial.read();
    // Convert to lowercase
    if (c >= 'A' && c <= 'Z') c += 32;

    // Ignore spaces and tabs
    if (c == ' ' || c == '\t') continue;

    // Store in buffer
    sharedData.serial_command_buffer[index++] = c;

    // Check for complete command
    if (c == '\n' || index >= 15) {
        sharedData.serial_command_buffer[index] = '\0';

        // Parse command
        if (strcmp(buffer, "on") == 0) {
            sharedData.actuator_command = true;
        } else if (strcmp(buffer, "off") == 0) {
            sharedData.actuator_command = false;
        } else if (strcmp(buffer, "toggle") == 0) {
            sharedData.actuator_command = !sharedData.actuator_command;
        } else if (strcmp(buffer, "status") == 0) {
            // Print status
        }

        index = 0;
    }
}
```

##### vTaskSignalConditioning - Signal Processing & Actuator Control

**Purpose**: Apply signal conditioning to raw actuator command and control hardware.

**Frequency**: 50ms (20Hz)

**Algorithm**:

```
1. Read actuator_command from SharedData
2. Apply signal conditioning:
   - Debounce (50ms)
   - Validation (100ms)
3. If conditioned state changed:
   - Update actuator (GPIO 23)
   - Update LED (GPIO 26)
   - Increment toggle count
4. Update SharedData.actuator_conditioned
```

**Signal Conditioning Application**:

```cpp
bool conditionedSignal = signalConditioner->conditionSignal(
    sharedData.actuator_command,
    ACTUATOR_DEBOUNCE_TIME_MS,      // 50ms
    ACTUATOR_VALIDATION_TIME_MS     // 100ms
);

// Check if state changed
if (conditionedSignal != sharedData.actuator_state) {
    sharedData.actuator_state = conditionedSignal;

    // Update hardware
    if (conditionedSignal) {
        actuator->turnOn();
        led->on();
    } else {
        actuator->turnOff();
        led->off();
    }

    // Increment toggle count
    sharedData.actuator_toggle_count++;
}
```

##### vTaskDisplay - LCD Updates

**Purpose**: Update the LCD display with current actuator state and statistics.

**Frequency**: 500ms (2Hz)

**Display Format**:
```
Line 1: "Actuator: ON/OFF"
Line 2: "Cmd: ON/OFF Tog:123"
```

**Implementation**:

```cpp
void vTaskDisplay(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];

        snprintf(line1, sizeof(line1), "Actuator: %s",
                 sharedData.actuator_state ? "ON" : "OFF");
        snprintf(line2, sizeof(line2), "Cmd: %s Tog:%lu",
                 sharedData.actuator_command ? "ON" : "OFF",
                 sharedData.actuator_toggle_count);

        if (lcdMutex.take(100)) {
            lcd->clear();
            kernel_primitives::delayMs(50);
            lcd->setCursor(0, 0);
            lcd->print(line1);
            kernel_primitives::delayMs(50);
            lcd->setCursor(0, 1);
            lcd->print(line2);
            lcdMutex.give();
        }

        // Generate 10-second status report
        static uint32_t lastReportTime = 0;
        if (xTaskGetTickCount() - lastReportTime >= pdMS_TO_TICKS(10000)) {
            lastReportTime = xTaskGetTickCount();
            printf("[DISPLAY] Status: Actuator=%s, Command=%s, Toggles=%lu\n",
                   sharedData.actuator_state ? "ON" : "OFF",
                   sharedData.actuator_command ? "ON" : "OFF",
                   sharedData.actuator_toggle_count);
        }
    }
}
```

### 5. Initialization (`init/init.cpp`)

The `setupApplication()` function initializes all components in sequence:

```cpp
void setupApplication() {
    // 1. Initialize Serial (115200 baud)
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    // 2. Print system information banner
    // ...

    // 3. Initialize Actuator
    actuator = new Actuator(ACTUATOR_PIN);
    actuator->begin();
    actuator->turnOff();

    // 4. Initialize Joystick
    joystick = new Joystick(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_SW_PIN);
    joystick->begin();

    // 5. Initialize LED
    led = new Led(LED_PIN);
    led->begin();
    led->off();

    // 6. Initialize Signal Conditioner
    signalConditioner = new SignalConditioner();

    // 7. Initialize LCD
    lcd = new LcdI2c(0x27, 16, 2);
    lcd->begin();
    lcd->setCursor(0, 0);
    lcd->print("Actuator Control");
    lcd->setCursor(0, 1);
    lcd->print("System Ready");

    // 8. Initialize SharedData
    initSharedData();

    // 9. Initialize Synchronization Primitives
    initSyncPrimitives();

    // 10. Create FreeRTOS Tasks
    createApplicationTasks();
}
```

## Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                vTaskActuatorControl (50ms)                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Read button state (GPIO 18)                           │  │
│  │ 2. Apply edge detection + 250ms cooldown                 │  │
│  │ 3. Read joystick button state (GPIO 25)                  │  │
│  │ 4. Check serial command buffer                           │  │
│  │ 5. Update SharedData.actuator_command                    │  │
│  │ 6. Update command timestamp                              │  │
│  │ 7. Signal semActuatorDisplay                             │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            │ Shared data access
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│              vTaskSignalConditioning (50ms)                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Read SharedData.actuator_command                     │  │
│  │ 2. Apply signal conditioning:                           │  │
│  │    - Debounce (50ms)                                    │  │
│  │    - Validation (100ms)                                 │  │
│  │ 3. Check if state changed                               │  │
│  │ 4. If changed:                                          │  │
│  │    - Update actuator (GPIO 23)                          │  │
│  │    - Update LED (GPIO 26)                               │  │
│  │    - Increment toggle count                             │  │
│  │ 5. Update SharedData.actuator_conditioned               │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            │ Shared data access
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                   vTaskDisplay (500ms)                          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Wait for 500ms (using vTaskDelayUntil)               │  │
│  │ 2. Read SharedData.actuator_state                       │  │
│  │ 3. Read SharedData.actuator_command                     │  │
│  │ 4. Read SharedData.actuator_toggle_count                │  │
│  │ 5. Format display strings                                │  │
│  │ 6. Acquire lcdMutex                                      │  │
│  │ 7. Update LCD:                                           │  │
│  │    Line 1: "Actuator: ON/OFF"                           │  │
│  │    Line 2: "Cmd: ON/OFF Tog:123"                        │  │
│  │ 8. Release lcdMutex                                      │  │
│  │ 9. Generate 10s status report                           │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Key Concepts Explained

### 1. Signal Conditioning

Signal conditioning improves robustness by filtering noise and ensuring stable signals:

**Stage 1: Debouncing (50ms)**
- Filters rapid electrical noise
- Eliminates mechanical button bounce
- Requires signal stability for 50ms before accepting

**Stage 2: Validation (100ms)**
- Confirms signal persistence
- Prevents transient false commands
- Total delay: 50ms + 100ms = 150ms max

**Example**:
```
Time:    0ms   10ms  20ms  30ms  40ms  50ms  60ms  70ms  80ms  90ms  100ms
Raw:     OFF   ON    OFF   ON    ON    ON    ON    ON    ON    ON    ON
Debounce: OFF   OFF   OFF   OFF   OFF   ON    ON    ON    ON    ON    ON
Validate: OFF   OFF   OFF   OFF   OFF   OFF   OFF   OFF   OFF   OFF   ON
```

### 2. Double-Click Prevention

Button cooldown prevents rapid toggling:

```cpp
if ((currentTime - lastButtonToggleTime) > 250) {
    // Allow toggle
} else {
    printf("Double-click prevented\n");
}
```

**Why 250ms?**
- Human reaction time: ~200ms
- Button bounce duration: < 50ms
- 250ms ensures user intended rapid toggle

### 3. Multiple Input Methods

The system supports three input methods:

1. **Physical Button (GPIO 18)**:
   - Simple toggle with edge detection
   - Internal pull-up resistor
   - 250ms cooldown

2. **Joystick Button (GPIO 25)**:
   - Alternative toggle method
   - Same cooldown mechanism
   - Useful for remote control

3. **Serial Commands**:
   - `on` - Turn actuator ON
   - `off` - Turn actuator OFF
   - `toggle` - Toggle current state
   - `status` - Print current status

### 4. Binary Semaphores

Binary semaphores signal the display task to update:

```cpp
// Signal from vTaskActuatorControl
semActuatorDisplay.give();

// Wait in vTaskDisplay
if (semActuatorDisplay.take(pdMS_TO_TICKS(100))) {
    // Update display
}
```

### 5. Mutex for Resource Protection

The mutex protects the LCD from concurrent access:

```cpp
if (lcdMutex.take(100)) {
    // Critical section
    lcd->print("Actuator: ON");
    lcdMutex.give();
}
```

### 6. FreeRTOS Task Priorities

Both control tasks have Priority 3 (highest), display has Priority 2:

```
Priority 3: vTaskActuatorControl (highest) - Input processing
Priority 3: vTaskSignalConditioning (highest) - Signal processing & hardware control
Priority 2: vTaskDisplay (medium) - LCD updates
```

**Why this priority order?**

1. **vTaskActuatorControl (Priority 3)**:
   - Critical for timely input processing
   - Must not miss button presses or serial commands
   - 50ms interval requires precise timing

2. **vTaskSignalConditioning (Priority 3)**:
   - Critical for real-time actuator control
   - Ensures signal conditioning is applied promptly
   - Direct hardware control

3. **vTaskDisplay (Priority 2)**:
   - Less critical than control
   - Updates every 500ms (tolerant to delays)
   - Can be preempted by control tasks

## Testing and Verification

### Expected Behavior

1. **Button Toggle**:
   - Press button (GPIO 18)
   - Actuator toggles after 150ms (50ms debounce + 100ms validation)
   - LED mirrors actuator state
   - LCD updates within 500ms

2. **Joystick Toggle**:
   - Press joystick button (GPIO 25)
   - Same behavior as button toggle
   - 250ms cooldown applies

3. **Serial Command "on"**:
   - Type "on" in serial monitor
   - Actuator turns ON after 150ms
   - LED turns ON
   - LCD shows "Actuator: ON"

4. **Serial Command "off"**:
   - Type "off" in serial monitor
   - Actuator turns OFF after 150ms
   - LED turns OFF
   - LCD shows "Actuator: OFF"

5. **Serial Command "toggle"**:
   - Type "toggle" in serial monitor
   - Actuator toggles current state
   - Same timing as on/off

6. **Double-Click Prevention**:
   - Rapid button presses (< 250ms apart)
   - Second press is ignored
   - Message: "Double-click prevented"

### Serial Output Example

```
=== LAB 4.1 - Actuator Control System ===
Actuator: GPIO 23
Button: GPIO 18
Joystick: X=34, Y=35, SW=25
LED: GPIO 26
LCD: I2C SDA=21, SCL=22 (0x27)
==========================================
Actuator initialized
Joystick initialized
LED initialized
Signal conditioner initialized
Initializing LCD...
LCD initialized
Creating FreeRTOS tasks...
=== FREE-RTOS SCHEDULER STARTED ===
Tasks running:
  - ActuatorControl (priority 3)
  - SignalConditioning (priority 3)
  - Display (priority 2)
=====================================
Actuator control active...

[ACTUATOR_CTRL] Button toggled to: ON
[SIG_COND] State changed: OFF -> ON
[SIG_COND] Actuator turned ON
[SIG_COND] LED turned ON
[DISPLAY] Status: Actuator=ON, Command=ON, Toggles=1

[ACTUATOR_CTRL] Button toggled to: OFF
[SIG_COND] State changed: ON -> OFF
[SIG_COND] Actuator turned OFF
[SIG_COND] LED turned OFF
[DISPLAY] Status: Actuator=OFF, Command=OFF, Toggles=2

[ACTUATOR_CTRL] Serial command: on
[ACTUATOR_CTRL] Command set to: ON
[SIG_COND] State changed: OFF -> ON
[SIG_COND] Actuator turned ON
[DISPLAY] Status: Actuator=ON, Command=ON, Toggles=3
```

## Configuration Parameters

All configurable parameters are defined in `state/state.cpp`:

```cpp
// GPIO Pin Assignments
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;
const uint8_t ACTUATOR_PIN = 23;
const uint8_t BUTTON_PIN = 18;
const uint8_t JOYSTICK_X_PIN = 34;
const uint8_t JOYSTICK_Y_PIN = 35;
const uint8_t JOYSTICK_SW_PIN = 25;
const uint8_t LED_PIN = 26;

// Task Configuration
const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_ACTUATOR_CTRL = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_SIGNAL_COND = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;

// Signal Conditioning Parameters
const uint32_t ACTUATOR_DEBOUNCE_TIME_MS = 50;
const uint32_t ACTUATOR_VALIDATION_TIME_MS = 100;
const uint32_t BUTTON_COOLDOWN_MS = 250;

// Task Periods
const uint32_t ACTUATOR_CONTROL_PERIOD_MS = 50;
const uint32_t SIGNAL_CONDITIONING_PERIOD_MS = 50;
const uint32_t DISPLAY_PERIOD_MS = 500;
```

### Adjusting Signal Conditioning

To change response speed:

```cpp
// Faster response (less filtering)
const uint32_t ACTUATOR_DEBOUNCE_TIME_MS = 20;
const uint32_t ACTUATOR_VALIDATION_TIME_MS = 50;

// Slower response (more filtering)
const uint32_t ACTUATOR_DEBOUNCE_TIME_MS = 100;
const uint32_t ACTUATOR_VALIDATION_TIME_MS = 200;
```

### Adjusting Button Cooldown

To change double-click prevention:

```cpp
// Shorter cooldown (allows faster toggling)
const uint32_t BUTTON_COOLDOWN_MS = 100;

// Longer cooldown (prevents accidental toggles)
const uint32_t BUTTON_COOLDOWN_MS = 500;
```

## Performance Characteristics

### Timing Analysis

| Component | Frequency | Period | CPU Usage |
|-----------|-----------|--------|-----------|
| Input Processing | 20 Hz | 50 ms | ~1-2% |
| Signal Conditioning | 20 Hz | 50 ms | ~1-2% |
| Display Update | 2 Hz | 500 ms | ~0.5% |
| **Total CPU Usage** | - | - | **~3-5%** |

### Memory Usage

```
RAM:   ~25,000 bytes (7.6% of 327,680 bytes)
Flash: ~300,000 bytes (22.9% of 1,310,720 bytes)
```

### Latency

- **Button to Actuator**: 150ms ± 20ms (50ms debounce + 100ms validation)
- **Serial to Actuator**: 150ms ± 20ms (command processing + conditioning)
- **Actuator to LCD**: < 650ms (conditioning + display cycle)
- **Maximum Toggle Rate**: ~4 toggles/second (limited by 250ms cooldown)

## Troubleshooting

### Issue: Actuator Not Responding

**Cause**: Signal conditioning timeout or wiring issue.

**Solution**:
1. Check actuator connections (GPIO 23)
2. Verify relay power supply
3. Monitor serial output for conditioning messages
4. Try serial commands ("on", "off")

### Issue: Double-Click Not Prevented

**Cause**: Cooldown timer not working.

**Solution**:
1. Verify BUTTON_COOLDOWN_MS value
2. Check xTaskGetTickCount() is working
3. Monitor serial for "Double-click prevented" message

### Issue: LCD Not Updating

**Cause**: Mutex not releasing or LCD not initializing.

**Solution**:
1. Verify I2C connections (SDA=21, SCL=22)
2. Check LCD address (should be 0x27)
3. Ensure lcdMutex.give() is called after lcdMutex.take()

### Issue: LED Not Mirroring Actuator

**Cause**: LED not updated in signal conditioning task.

**Solution**:
1. Check LED pin (GPIO 26)
2. Verify LED is updated when actuator state changes
3. Check for electrical issues (resistor, ground)

## Comparison with Lab 3.1 (Sound Detection)

| Feature | Lab 3.1 (Sound Detection) | Lab 4.1 (Actuator Control) |
|---------|---------------------------|----------------------------|
| Input Method | Sound sensor (ADC + digital) | Button, joystick, serial |
| Control Target | LED (1-second pulse) | Actuator/relay (persistent) |
| Signal Processing | Threshold + hysteresis | Debounce + validation |
| Task Count | 3 (Detect, Display, LED) | 3 (ActuatorCtrl, SignalCond, Display) |
| Timing | 20ms, 500ms, continuous | 50ms, 50ms, 500ms |
| Synchronization | 2 semaphores, 1 mutex | 1 semaphore, 1 mutex |
| Output | LED only | Actuator + LED + LCD |
| Response Time | < 20ms | 150ms (conditioning) |

## Conclusion

This implementation demonstrates:

1. **Robust signal conditioning** with debouncing and validation
2. **Multiple input methods** (button, joystick, serial)
3. **Real-time actuator control** using FreeRTOS
4. **Task synchronization** using semaphores and mutex
5. **Hardware abstraction** through modular drivers
6. **Double-click prevention** for reliable button operation
7. **Visual feedback** via LED and LCD display

The system provides a solid foundation for understanding real-time control systems with signal conditioning and can be extended with:
- PWM control for variable actuators
- Multiple actuators with coordination
- Sensor feedback (position, current, temperature)
- Network control (WiFi, Bluetooth)
- Advanced signal processing (filtering, PID control)

## Future Enhancements

Potential improvements could include:

1. **Configurable parameters** via EEPROM or serial
2. **PWM actuator control** for variable speed/position
3. **Multiple actuators** with coordination logic
4. **Sensor feedback** (limit switches, encoders)
5. **Safety interlocks** and emergency stop
6. **Network control** via WiFi/Bluetooth
7. **Data logging** to SD card
8. **Advanced signal processing** (filtering, PID)
9. **Actuator diagnostics** (current monitoring, fault detection)
10. **Web interface** for remote control and monitoring