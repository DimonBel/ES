# Sound Detection System Implementation Documentation

## Overview

This document provides a comprehensive explanation of the Sound Detection System implementation for Lab 2.2. The system uses FreeRTOS on ESP32 to detect sound signals, implement threshold-based alerting with hysteresis, and provide visual feedback via LED and LCD display.

## System Architecture

### Hardware Components

| Component | GPIO Pin | Type | Function |
|-----------|----------|------|----------|
| Sound Sensor D0 | 12 | Digital Input | Digital sound detection (HIGH when sound detected) |
| Sound Sensor A0 | 32 | Analog Input | Analog sound level reading (0-4095) |
| LED | 14 | Digital Output | Visual indicator (1-second pulse on detection) |
| LCD SDA | 21 | I2C Data | Display communication |
| LCD SCL | 22 | I2C Clock | Display communication |
| Serial | TX0/RX0 | UART | Debug output (115200 baud) |

### Sound Sensor Configuration

- **Threshold**: 2000 (analog value 0-4095)
- **Hysteresis**: 100 (anti-bounce margin)
- **Debounce Time**: 50ms
- **Acquisition Rate**: 20ms (50Hz)

## Software Architecture

### Module Structure

```
src/modules/
├── freertos_app/          # FreeRTOS application framework
│   ├── init/             # Hardware initialization & task creation
│   ├── state/            # Shared data & configuration
│   ├── sync/             # Synchronization primitives
│   └── tasks/            # FreeRTOS tasks implementation
├── sound_sensor/         # Sound sensor driver
├── led/                  # LED control module
├── lcd/                  # I2C LCD display module
└── kernel_primitives/    # FreeRTOS wrappers
    ├── task/
    ├── mutex/
    └── semaphore/
```

### FreeRTOS Tasks

The system uses three FreeRTOS tasks with different priorities:

| Task | Priority | Stack Size | Frequency | Purpose |
|------|----------|------------|-----------|---------|
| vTaskDetect | 3 (highest) | 4096 bytes | 20ms | Sound sensor acquisition & processing |
| vTaskDisplay | 2 (medium) | 4096 bytes | 500ms | LCD display updates |
| vTaskLED | 1 (lowest) | 4096 bytes | Continuous | LED control with timeout |

## Detailed Implementation

### 1. Sound Sensor Module (`sound_sensor/`)

**File**: `sound_sensor.h`, `sound_sensor.cpp`

The SoundSensor class provides an interface for reading both digital and analog signals from the sound sensor.

#### Public API

```cpp
class SoundSensor {
public:
    // Constructor
    explicit SoundSensor(uint8_t digitalPin, uint8_t analogPin);
    
    // Initialization
    void begin();
    
    // Reading methods
    bool readDigital();              // Returns HIGH/LOW
    uint16_t readAnalog();           // Returns 0-4095
    bool isSoundDetected();          // Edge detection (rising)
    uint16_t getAnalogValue();       // Returns cached value
    
    // Configuration
    void setThreshold(uint16_t threshold);    // Default: 2000
    void setHysteresis(uint16_t hysteresis);  // Default: 100
};
```

#### Key Features

1. **Digital Reading**: Reads the D0 pin which outputs HIGH when sound exceeds the sensor's internal threshold
2. **Analog Reading**: Reads the A0 pin which provides continuous analog values (0-4095) representing sound intensity
3. **Edge Detection**: `isSoundDetected()` implements rising edge detection to detect new sound events
4. **Threshold Configuration**: Allows runtime adjustment of detection threshold and hysteresis

#### Edge Detection Implementation

```cpp
bool SoundSensor::isSoundDetected() {
    bool currentDigitalState = readDigital();
    
    // Detect rising edge (transition from LOW to HIGH)
    if (currentDigitalState && !_lastDigitalState) {
        _lastDigitalState = currentDigitalState;
        return true;
    }
    
    _lastDigitalState = currentDigitalState;
    return false;
}
```

### 2. FreeRTOS Application Framework (`freertos_app/`)

#### 2.1 Shared Data Structure (`state/state.h`)

```cpp
struct SharedData {
    uint16_t analog_value;           // Current sound sensor analog value
    bool sound_detected;             // Digital detection flag
    bool threshold_exceeded;         // Threshold exceeded flag (with hysteresis)
    uint32_t sound_count;            // Total sound detection count
    uint8_t task_state;              // Task state indicator
    uint32_t last_sound_time;        // Last detection timestamp (ticks)
    bool led_state;                  // LED on/off state
    uint32_t led_turn_off_time;      // LED timeout timestamp (ticks)
};
```

This structure is shared between all tasks and provides thread-safe access to system state.

#### 2.2 Synchronization Primitives (`sync/sync.cpp`)

```cpp
// Protects LCD access from concurrent tasks
kernel_primitives::Mutex lcdMutex;

// Signals display task to update LCD
kernel_primitives::BinarySemaphore semSoundDisplay;

// Signals LED task to turn on LED
kernel_primitives::BinarySemaphore semSoundLED;
```

The `updateLCD()` function uses mutex to ensure only one task accesses the LCD at a time:

```cpp
void updateLCD(const char *line1, const char *line2) {
    if (lcd == nullptr) return;

    if (lcdMutex.take(100)) {          // Try to acquire mutex (100ms timeout)
        lcd->clear();
        kernel_primitives::delayMs(50);
        lcd->setCursor(0, 0);
        lcd->print(line1);
        kernel_primitives::delayMs(50);
        lcd->setCursor(0, 1);
        lcd->print(line2);
        lcdMutex.give();                // Release mutex
    }
}
```

#### 2.3 Task Implementation (`tasks/tasks.cpp`)

##### vTaskDetect - Sensor Acquisition & Processing

**Purpose**: Continuously acquire sound data and implement threshold-based detection with hysteresis.

**Frequency**: 20ms (50Hz)

**Algorithm**:

```
1. Read analog value from sound sensor (0-4095)
2. Read digital value for edge detection
3. Apply threshold with hysteresis:
   - If LED is ON: require value < (threshold - hysteresis) to turn OFF
   - If LED is OFF: require value > threshold to turn ON
4. Debounce: only change state if stable for minimum time (50ms)
5. Update shared data
6. Signal display and LED tasks via semaphores
```

**Hysteresis Implementation**:

```cpp
bool currentThresholdState = false;
if (sharedData.led_state) {
    // LED is ON, require value below (threshold - hysteresis) to turn off
    currentThresholdState = (currentValue > (SOUND_THRESHOLD - SOUND_HYSTERESIS));
} else {
    // LED is OFF, require value above threshold to turn on
    currentThresholdState = (currentValue > SOUND_THRESHOLD);
}
```

**Why Hysteresis?**

Without hysteresis, when the analog value hovers near the threshold (e.g., 1999-2001), the system would rapidly toggle between ON and OFF states, causing:
- LED flickering
- Multiple false alerts
- Unstable system behavior

Hysteresis creates a "dead band" where the system maintains its current state, preventing oscillation.

**Debounce Mechanism**:

```cpp
if (currentThresholdState != sharedData.threshold_exceeded) {
    if (soundDetected || (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(SOUND_DEBOUNCE_TIME))) {
        // Only change state if sound detected OR debounce time elapsed
        sharedData.threshold_exceeded = currentThresholdState;
        // ... update state and signal tasks
    }
}
```

##### vTaskDisplay - LCD Updates

**Purpose**: Update the LCD display with current sound count.

**Frequency**: 500ms (2Hz)

**Display Format**:
```
Line 1: "Count:"
Line 2: "<number>"
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

        snprintf(line1, sizeof(line1), "Count:");
        snprintf(line2, sizeof(line2), "%lu", sharedData.sound_count);

        updateLCD(line1, line2);
    }
}
```

**Why vTaskDelayUntil?**

Using `vTaskDelayUntil()` instead of `vTaskDelay()` ensures precise timing:
- `vTaskDelay()`: Waits for specified time from when function is called
- `vTaskDelayUntil()`: Waits until a specific absolute time, compensating for execution time

This ensures the display updates exactly every 500ms, regardless of processing time.

##### vTaskLED - LED Control

**Purpose**: Control LED with 1-second pulse behavior on sound detection.

**Frequency**: Continuous (runs in loop)

**Behavior**:

1. When sound is detected (via semaphore), turn LED ON
2. After exactly 1 second, automatically turn LED OFF
3. Implements timeout mechanism using tick counting

**Implementation**:

```cpp
void vTaskLED(void *pvParameters) {
    for (;;) {
        uint32_t currentTime = xTaskGetTickCount();
        
        // Check if LED should be turned off (1-second timeout)
        if (sharedData.led_state && currentTime >= sharedData.led_turn_off_time) {
            sharedData.led_state = false;
            if (led) {
                led->off();
                printf("[LED] LED OFF (timeout)\n");
            }
        }

        // Turn on LED when signal received
        if (semSoundLED.take(0)) {
            if (led && sharedData.led_state) {
                led->on();
                printf("[LED] LED ON (sound detected)\n");
            }
        }

        kernel_primitives::delayMs(50);
    }
}
```

**1-Second Pulse Mechanism**:

When sound is detected:
```cpp
sharedData.led_state = true;
sharedData.led_turn_off_time = currentTime + pdMS_TO_TICKS(1000);
```

The LED task continuously checks if the current time exceeds `led_turn_off_time` and automatically turns off the LED.

### 3. Initialization (`init/init.cpp`)

The `setupApplication()` function initializes all components in sequence:

```cpp
void setupApplication() {
    // 1. Initialize Serial (115200 baud)
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    // 2. Initialize LED
    led = new Led(LED_PIN);
    led->begin();
    led->off();

    // 3. Initialize Sound Sensor
    soundSensor = new SoundSensor(SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    soundSensor->begin();
    soundSensor->setThreshold(SOUND_THRESHOLD);
    soundSensor->setHysteresis(SOUND_HYSTERESIS);

    // 4. Initialize LCD
    lcd = new LcdI2c(0x27, 16, 2);
    lcd->begin();
    lcd->setCursor(0, 0);
    lcd->print("Sound Detection");
    lcd->setCursor(0, 1);
    lcd->print("System Ready");

    // 5. Initialize Synchronization Primitives
    initSyncPrimitives();

    // 6. Create FreeRTOS Tasks
    createApplicationTasks();
}
```

## Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                     vTaskDetect (20ms)                          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Read analog value from A0 (0-4095)                    │  │
│  │ 2. Read digital value from D0                            │  │
│  │ 3. Apply threshold + hysteresis logic                    │  │
│  │ 4. Check debounce time (50ms)                            │  │
│  │ 5. Update SharedData:                                    │  │
│  │    - analog_value                                        │  │
│  │    - sound_count (increment)                             │  │
│  │    - led_state (true)                                    │  │
│  │    - led_turn_off_time (current + 1000ms)               │  │
│  │ 6. Signal semSoundDisplay                                │  │
│  │ 7. Signal semSoundLED                                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            │
                            │ semSoundDisplay (non-blocking)
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                   vTaskDisplay (500ms)                          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Wait for 500ms (using vTaskDelayUntil)               │  │
│  │ 2. Read SharedData.sound_count                          │  │
│  │ 3. Format: "Count:" + number                             │  │
│  │ 4. Acquire lcdMutex                                      │  │
│  │ 5. Update LCD:                                           │  │
│  │    Line 1: "Count:"                                      │  │
│  │    Line 2: "<number>"                                    │  │
│  │ 6. Release lcdMutex                                      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘

                            │
                            │ semSoundLED (non-blocking)
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                     vTaskLED (continuous)                       │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Check if semSoundLED signaled                         │  │
│  │    - If yes: Turn LED ON                                 │  │
│  │ 2. Check if led_turn_off_time reached                    │  │
│  │    - If yes: Turn LED OFF                                │  │
│  │ 3. Delay 50ms                                            │  │
│  │ 4. Repeat                                                │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Key Concepts Explained

### 1. Threshold-Based Detection

The system uses threshold detection to determine if sound levels are significant:

```
if (analog_value > threshold) {
    // Sound detected
} else {
    // Normal sound level
}
```

**Example**:
- Threshold: 2000
- Current value: 2831 → **DETECTED** (2831 > 2000)
- Current value: 1271 → **NOT DETECTED** (1271 < 2000)

### 2. Hysteresis (Anti-Bounce)

Hysteresis prevents rapid toggling when the signal is near the threshold:

```
Turn ON condition:  value > threshold
Turn OFF condition: value < (threshold - hysteresis)
```

**Example** (threshold=2000, hysteresis=100):
- LED is OFF, value=1990 → **STAYS OFF** (1990 < 2000)
- LED is OFF, value=2001 → **TURNS ON** (2001 > 2000)
- LED is ON, value=1950 → **STAYS ON** (1950 > 1900)
- LED is ON, value=1890 → **TURNS OFF** (1890 < 1900)

**Benefits**:
- Prevents LED flickering
- Reduces false positives
- Provides stable output

### 3. Debouncing

Debouncing ensures the signal is stable before changing state:

```cpp
if (currentTime - last_sound_time >= debounce_time) {
    // State change is valid
    last_sound_time = currentTime;
    // ... proceed with state change
}
```

**Purpose**: Ignores transient signals that don't persist for the minimum debounce time.

### 4. Binary Semaphores

Binary semaphores are used for task synchronization:

```cpp
// Signal from vTaskDetect to vTaskDisplay
semSoundDisplay.give();  // Signal that display needs update

// Wait in vTaskDisplay
if (semSoundDisplay.take(0)) {  // Non-blocking check
    // Update display
}
```

**Why non-blocking take(0)?**
- vTaskDisplay updates every 500ms regardless
- If semaphore signaled, display updates immediately
- If not, regular periodic update continues

### 5. Mutex for Resource Protection

The mutex protects the LCD from concurrent access:

```cpp
// Task A trying to update LCD
if (lcdMutex.take(100)) {
    // Critical section - exclusive LCD access
    lcd->print("Hello");
    lcdMutex.give();
}

// Task B trying to update LCD simultaneously
if (lcdMutex.take(100)) {
    // Waits until Task A releases mutex
    lcd->print("World");
    lcdMutex.give();
}
```

**Without mutex**: Display corruption could occur if tasks write simultaneously.

### 6. FreeRTOS Task Priorities

Higher priority tasks preempt lower priority tasks:

```
Priority 3: vTaskDetect (highest) - Always runs when ready
Priority 2: vTaskDisplay (medium) - Runs when Detect is blocked
Priority 1: vTaskLED (lowest) - Runs when Detect and Display are blocked
```

**Why this priority order?**

1. **vTaskDetect (Priority 3)**:
   - Critical for real-time data acquisition
   - Must not miss sound events
   - 20ms interval requires precise timing

2. **vTaskDisplay (Priority 2)**:
   - Less critical than detection
   - Updates every 500ms (tolerant to slight delays)
   - Medium priority ensures it runs regularly

3. **vTaskLED (Priority 1)**:
   - Lowest criticality
   - LED timing is forgiving (1-second pulse)
   - Continuous operation, can be delayed slightly

## Testing and Verification

### Expected Behavior

1. **Normal State (No Sound)**:
   - LCD: "Count:" + current count
   - LED: OFF
   - Serial: "Monitoring..." messages

2. **Sound Detected**:
   - LCD: Count increments
   - LED: Turns ON for 1 second, then OFF
   - Serial: "[DETECT] Sound detected!" + count

3. **Multiple Rapid Sounds**:
   - Count increments for each detection
   - LED pulses for each detection (may overlap if < 1 second apart)
   - LCD updates with new count every 500ms

### Serial Output Example

```
=== LAB 2.2 - Sound Detection System ===
Sound Sensor: D0=12, A0=32
LED: 14
LCD: I2C SDA=21, SCL=22 (0x27)
==========================================
LED initialized
Sound sensor initialized
  Threshold: 2000
  Hysteresis: 100
Testing LED...
LED test complete
Initializing LCD...
LCD initialized
Creating FreeRTOS tasks...
=== FREE-RTOS SCHEDULER STARTED ===
Tasks running:
  - Detect (priority 3)
  - Display (priority 2)
  - LED (priority 1)
=====================================
Sound detection active...

[DETECT] Sound detected! Analog: 2831, Threshold: 2000
[DETECT] Digital sound detected! Count: 1
[LED] LED ON (sound detected)
[LED] LED OFF (timeout)
[DETECT] Sound detected! Analog: 2681, Threshold: 2000
[DETECT] Digital sound detected! Count: 2
[LED] LED ON (sound detected)
[LED] LED OFF (timeout)
```

## Configuration Parameters

All configurable parameters are defined in `state/state.cpp`:

```cpp
// GPIO Pin Assignments
const uint8_t SOUND_SENSOR_D0_PIN = 12;
const uint8_t SOUND_SENSOR_A0_PIN = 32;
const uint8_t LED_PIN = 14;
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Task Configuration
const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DETECT = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_LED = tskIDLE_PRIORITY + 1;

// Sound Detection Parameters
const uint16_t SOUND_THRESHOLD = 2000;    // Analog threshold (0-4095)
const uint16_t SOUND_HYSTERESIS = 100;    // Anti-bounce margin
const uint32_t SOUND_DEBOUNCE_TIME = 50;  // Debounce delay (ms)
```

### Adjusting Threshold

To make the system more/less sensitive:

```cpp
// More sensitive (detects quieter sounds)
const uint16_t SOUND_THRESHOLD = 1500;

// Less sensitive (only detects loud sounds)
const uint16_t SOUND_THRESHOLD = 2500;
```

### Adjusting Hysteresis

To reduce/increase dead band:

```cpp
// Smaller hysteresis (more responsive, less stable)
const uint16_t SOUND_HYSTERESIS = 50;

// Larger hysteresis (less responsive, more stable)
const uint16_t SOUND_HYSTERESIS = 200;
```

### Adjusting Debounce Time

To change minimum signal duration:

```cpp
// Shorter debounce (responds faster, more false positives)
const uint32_t SOUND_DEBOUNCE_TIME = 20;

// Longer debounce (responds slower, fewer false positives)
const uint32_t SOUND_DEBOUNCE_TIME = 100;
```

## Performance Characteristics

### Timing Analysis

| Component | Frequency | Period | CPU Usage |
|-----------|-----------|--------|-----------|
| Sensor Acquisition | 50 Hz | 20 ms | ~1-2% |
| Display Update | 2 Hz | 500 ms | ~0.5% |
| LED Control | Continuous | 50 ms loop | ~0.5% |
| **Total CPU Usage** | - | - | **~3-4%** |

### Memory Usage

```
RAM:   22,016 bytes (6.7% of 327,680 bytes)
Flash: 297,541 bytes (22.7% of 1,310,720 bytes)
```

### Latency

- **Sensor to LED**: < 20ms (one detect cycle)
- **Sensor to LCD**: < 520ms (detect + display cycle)
- **LED Pulse Duration**: 1000ms ± 50ms

## Troubleshooting

### Issue: LED Stays ON Permanently

**Cause**: `led_turn_off_time` not being checked or set correctly.

**Solution**: Verify vTaskLED is running and checking timeout properly.

### Issue: Count Not Incrementing

**Cause**: Sound detection not triggering or `sound_count` not being updated.

**Solution**: 
1. Check sound sensor connections
2. Verify threshold value is appropriate
3. Monitor serial output for detection messages

### Issue: LCD Not Updating

**Cause**: Mutex not releasing or LCD not initializing.

**Solution**:
1. Verify I2C connections (SDA=21, SCL=22)
2. Check LCD address (should be 0x27)
3. Ensure lcdMutex.give() is called after lcdMutex.take()

### Issue: Rapid LED Flickering

**Cause**: Hysteresis too small or threshold inappropriate.

**Solution**: Increase hysteresis value or adjust threshold.

## Conclusion

This implementation demonstrates:

1. **Real-time signal acquisition** using FreeRTOS tasks
2. **Threshold-based detection** with hysteresis for stability
3. **Task synchronization** using semaphores and mutex
4. **Hardware abstraction** through modular sensor drivers
5. **Efficient resource usage** with minimal CPU overhead

The system provides a robust foundation for sound detection applications and can be extended with additional sensors, more complex algorithms, or different output mechanisms.

## Future Enhancements

Potential improvements could include:

1. **Configurable parameters** via serial commands or keypad
2. **Multiple thresholds** for different alert levels
3. **Sound pattern recognition** for specific sounds
4. **Data logging** to SD card or EEPROM
5. **Wireless transmission** via WiFi/Bluetooth
6. **Multi-sensor fusion** with other sensors (motion, temperature)
7. **Adaptive threshold** based on ambient noise level
8. **Sound level meter** display (dB conversion)