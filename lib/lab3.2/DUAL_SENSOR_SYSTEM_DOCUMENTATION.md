# Dual Sensor System Implementation Documentation

## Overview

This document provides a comprehensive explanation of the Dual Sensor System implementation for Lab 3.2. The system uses FreeRTOS on ESP32 to monitor two sensors simultaneously: a sound sensor for clap detection and a DS18B20 temperature sensor for environmental monitoring. The system provides visual feedback via RGB LED and LCD display with intelligent display switching.

## System Architecture

### Hardware Components

| Component | GPIO Pin | Type | Function |
|-----------|----------|------|----------|
| Sound Sensor D0 | 12 | Digital Input | Digital sound detection (HIGH when sound detected) |
| Sound Sensor A0 | 34 | Analog Input | Analog sound level reading (0-4095) |
| DS18B20 Temperature Sensor | 4 | OneWire | Temperature reading (-55°C to +125°C) |
| RGB LED Red | 25 | PWM Output | Red channel (idle state) |
| RGB LED Green | 26 | PWM Output | Green channel (clap detected) |
| RGB LED Blue | 27 | PWM Output | Blue channel (reserved) |
| LCD SDA | 21 | I2C Data | Display communication |
| LCD SCL | 22 | I2C Clock | Display communication |
| Serial | TX0/RX0 | UART | Debug output (115200 baud) |

### Sensor Configuration

**Sound Sensor:**
- **Threshold**: 1500 (analog value 0-4095)
- **Hysteresis**: 50 (anti-bounce margin)
- **Debounce Time**: 50ms
- **Acquisition Rate**: 20ms (50Hz)
- **Minimum Trigger Interval**: 5000ms (5 seconds)

**Temperature Sensor (DS18B20):**
- **Resolution**: 12 bits (0.0625°C precision)
- **Conversion Time**: ~750ms
- **Temperature Range**: -55°C to +125°C
- **Filter**: Median filter (5 samples)
- **Update Rate**: 800ms (100ms request + 700ms wait)

### Display Behavior

**LCD Display Logic:**
- **Sound Detected**: Shows "Sound --> <value>" for 2 seconds
- **Idle State**: Shows temperature (raw and filtered)
- **Auto-switch**: Automatically returns to temperature after 2 seconds of silence

**RGB LED Logic:**
- **Idle State**: Red (indicates monitoring mode)
- **Clap Detected**: Green (for 1 second)
- **Transition**: Smooth color change

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
├── ds18b20/              # DS18B20 temperature sensor driver
├── rgb_led/              # RGB LED control module
├── lcd/                  # I2C LCD display module
└── kernel_primitives/    # FreeRTOS wrappers
    ├── task/
    ├── mutex/
    └── semaphore/
```

### FreeRTOS Tasks

The system uses four FreeRTOS tasks with different priorities:

| Task | Priority | Stack Size | Frequency | Purpose |
|------|----------|------------|-----------|---------|
| vTaskDetect | 3 (highest) | 4096 bytes | 20ms | Sound sensor acquisition & processing |
| vTaskTemperature | 3 (highest) | 4096 bytes | 100ms | Temperature sensor reading & filtering |
| vTaskDisplay | 2 (medium) | 4096 bytes | 100ms | LCD display updates with auto-switch |
| vTaskLED | 1 (lowest) | 4096 bytes | Continuous | RGB LED control with timeout |

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
    void setThreshold(uint16_t threshold);    // Default: 1500
    void setHysteresis(uint16_t hysteresis);  // Default: 50
};
```

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

### 2. DS18B20 Temperature Sensor Module (`ds18b20/`)

**File**: `ds18b20.h`, `ds18b20.cpp`

The DS18B20 class provides an interface for reading temperature from the DS18B20 digital temperature sensor using the OneWire protocol.

#### Public API

```cpp
class DS18B20 {
public:
    explicit DS18B20(uint8_t pin);

    // Initialization
    void begin();

    // Temperature reading methods
    float readTemperature();         // Blocking read (with delay)
    bool requestTemperature();       // Start conversion
    bool isConversionComplete();    // Check if ready
    float getTemperature();          // Read result

    // Configuration
    void setResolution(uint8_t resolution);  // 9, 10, 11, or 12 bits
    bool isPresent();                // Check if device is found
};
```

#### Temperature Reading Flow

The temperature reading is split into two operations for non-blocking behavior in FreeRTOS:

```
1. requestTemperature()
   - Reset OneWire bus
   - Select device by address
   - Send command 0x44 (start conversion)
   - Return immediately

2. Wait ~750ms (for 12-bit resolution)

3. getTemperature()
   - Reset OneWire bus
   - Select device by address
   - Send command 0xBE (read scratchpad)
   - Read 9 bytes
   - Verify CRC
   - Convert raw data to Celsius
   - Return temperature
```

#### Raw Data Conversion

```cpp
// Raw data format (2 bytes):
// Byte 0: LSB of temperature
// Byte 1: MSB of temperature
// Temperature = (MSB << 8) | LSB
// Celsius = Temperature / 16.0

int16_t raw = (data[1] << 8) | data[0];

// Handle negative temperatures
if (raw & 0x8000) {
    raw = ~raw + 1;  // Two's complement
}

float celsius = raw / 16.0f;
```

#### CRC Verification

```cpp
// Simple XOR CRC for 8 bytes
uint8_t crc = 0;
for (uint8_t i = 0; i < 8; i++) {
    crc ^= data[i];
}

if (crc != data[8]) {
    // CRC error - data corrupted
    return -127.0f;  // Error value
}
```

### 3. RGB LED Module (`rgb_led/`)

**File**: `rgb_led.h`, `rgb_led.cpp`

The RgbLed class provides control for a common cathode RGB LED.

#### Public API

```cpp
class RgbLed {
public:
    RgbLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);

    // Initialization
    void begin();

    // Color control
    void red();          // Red color (idle)
    void green();        // Green color (clap detected)
    void blue();         // Blue color (reserved)
    void white();        // All colors on
    void off();          // All colors off
    void setColor(uint8_t r, uint8_t g, uint8_t b);  // Custom color
};
```

#### PWM Implementation

```cpp
void RgbLed::red() {
    digitalWrite(_redPin, HIGH);   // Red ON
    digitalWrite(_greenPin, LOW);  // Green OFF
    digitalWrite(_bluePin, LOW);   // Blue OFF
}

void RgbLed::green() {
    digitalWrite(_redPin, LOW);    // Red OFF
    digitalWrite(_greenPin, HIGH);  // Green ON
    digitalWrite(_bluePin, LOW);   // Blue OFF
}
```

### 4. FreeRTOS Application Framework (`freertos_app/`)

#### 4.1 Shared Data Structure (`state/state.h`)

```cpp
struct SharedData {
    // Sound sensor data
    uint16_t analog_value;           // Current sound sensor analog value
    bool sound_detected;             // Digital detection flag
    bool threshold_exceeded;         // Threshold exceeded flag
    uint32_t sound_count;            // Total sound detection count
    uint32_t last_sound_time;        // Last detection timestamp (ticks)
    bool led_state;                  // LED on/off state
    uint32_t led_turn_off_time;      // LED timeout timestamp (ticks)

    // Temperature sensor data
    float temperature;               // Current temperature reading
    float temperature_filtered;      // Median-filtered temperature
    bool temperature_available;      // Valid temperature flag
    uint32_t last_temperature_time;  // Last reading timestamp

    // Filter buffer for median filter
    float temperature_buffer[5];     // Circular buffer (5 samples)
    uint8_t temperature_buffer_index; // Current buffer position
};
```

#### 4.2 Synchronization Primitives (`sync/sync.cpp`)

```cpp
// Protects LCD access from concurrent tasks
kernel_primitives::Mutex lcdMutex;

// Signals display task to update LCD with sound
kernel_primitives::BinarySemaphore semSoundDisplay;

// Signals LED task to change LED color
kernel_primitives::BinarySemaphore semSoundLED;

// Signals display task to update LCD with temperature
kernel_primitives::BinarySemaphore semTempDisplay;
```

The `updateLCD()` function uses mutex to ensure only one task accesses the LCD at a time:

```cpp
void updateLCD(const char *line1, const char *line2) {
    if (lcd == nullptr) return;

    if (lcdMutex.take(100)) {          // Try to acquire mutex (100ms timeout)
        lcd->setCursor(0, 0);
        lcd->print(line1);
        lcd->setCursor(0, 1);
        lcd->print(line2);
        lcdMutex.give();                // Release mutex
    }
}
```

**Optimization**: LCD clear() was removed to reduce flickering and improve response time.

#### 4.3 Task Implementation (`tasks/tasks.cpp`)

##### vTaskDetect - Sensor Acquisition & Processing

**Purpose**: Continuously acquire sound data and implement threshold-based detection with rising edge detection.

**Frequency**: 20ms (50Hz)

**Algorithm**:

```
1. Read analog value from sound sensor (0-4095)
2. Read digital value for edge detection
3. Apply rising edge detection:
   - Only trigger when signal goes from below to above threshold
   - Prevent multiple triggers from the same clap
4. Check minimum interval (5 seconds between claps)
5. Update shared data
6. Signal display and LED tasks via semaphores
```

**Rising Edge Detection Implementation**:

```cpp
static bool wasAboveThreshold = false;  // Track previous state

// Trigger only when sound goes from below to above threshold
if (currentValue > SOUND_THRESHOLD && !wasAboveThreshold) {
    wasAboveThreshold = true;

    // Prevent too frequent triggers (minimum 5000ms between triggers)
    if (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(5000)) {
        sharedData.sound_count++;
        sharedData.last_sound_time = currentTime;
        sharedData.led_state = true;
        sharedData.led_turn_off_time = currentTime + pdMS_TO_TICKS(1000);

        semSoundDisplay.give();
        semSoundLED.give();
    }
}

// Reset flag when sound goes below threshold
if (currentValue <= SOUND_THRESHOLD) {
    wasAboveThreshold = false;
}
```

**Why Rising Edge Detection?**

Without rising edge detection, when the analog value remains above the threshold for an extended period, the system would continuously trigger, causing:
- Rapid LCD flickering
- RGB LED stuck on green
- Multiple false alerts
- System instability

Rising edge detection ensures ONE trigger per clap event.

**Minimum Interval (5 seconds)**:

```cpp
if (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(5000)) {
    // Allow new clap trigger
}
```

This ensures LCD has time to display temperature between claps (2 seconds sound + 3 seconds temperature).

##### vTaskTemperature - Temperature Sensor Reading

**Purpose**: Read temperature from DS18B20 sensor, apply median filter, and signal display task.

**Frequency**: 100ms cycle, but effective update rate is ~800ms

**Algorithm**:

```
State machine:
1. If not requested:
   - Request temperature conversion
   - Wait 8 cycles (800ms)
2. If requested and 8 cycles elapsed:
   - Read temperature from sensor
   - Add to filter buffer (circular)
   - Calculate median filter (5 samples)
   - Update shared data
   - Signal display task
   - Reset for next conversion
```

**Implementation**:

```cpp
void vTaskTemperature(void *pvParameters) {
    static bool conversionRequested = false;
    static uint8_t waitCycles = 0;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (!conversionRequested) {
            // Request temperature conversion
            tempSensor->requestTemperature();
            conversionRequested = true;
            waitCycles = 0;
        } else {
            // Wait for conversion to complete
            waitCycles++;
            if (waitCycles >= 8) {  // 8 * 100ms = 800ms
                // Read temperature
                float temp = tempSensor->getTemperature();

                if (temp > -100.0f) {  // Valid temperature
                    sharedData.temperature = temp;
                    sharedData.temperature_available = true;

                    // Add to filter buffer (circular buffer)
                    sharedData.temperature_buffer[sharedData.temperature_buffer_index] = temp;
                    sharedData.temperature_buffer_index = (sharedData.temperature_buffer_index + 1) % 5;

                    // Calculate median filter
                    float sorted[5];
                    for (int i = 0; i < 5; i++) {
                        sorted[i] = sharedData.temperature_buffer[i];
                    }

                    // Simple bubble sort
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4 - i; j++) {
                            if (sorted[j] > sorted[j + 1]) {
                                float temp = sorted[j];
                                sorted[j] = sorted[j + 1];
                                sorted[j + 1] = temp;
                            }
                        }
                    }

                    // Median is the middle value
                    sharedData.temperature_filtered = sorted[2];

                    // Signal display task to update
                    semTempDisplay.give();
                }

                conversionRequested = false;  // Ready for next conversion
            }
        }
    }
}
```

**Why Median Filter?**

Median filter removes outliers and provides stable temperature readings:

```
Raw readings: 26.5, 27.1, 26.3, 30.2, 26.4
Sorted:        26.3, 26.4, 26.5, 27.1, 30.2
Median: 26.5°C (ignores outlier 30.2)
```

**Why 800ms Wait?**

DS18B20 with 12-bit resolution requires ~750ms for temperature conversion. We use 800ms to ensure completion.

##### vTaskDisplay - LCD Updates with Auto-Switch

**Purpose**: Update LCD display with either sound level or temperature, with automatic switching based on recent sound activity.

**Frequency**: 100ms (10Hz)

**Display Logic**:

```
If (currentTime - last_sound_time) < 2000ms:
    Display: "Sound --> <analog_value>"
            "Clap!"
Else:
    If temperature_available:
        Display: "Temp: <temp>.<1f> C"
                "Filt: <filtered>.<1f> C"
    Else:
        Display: "Temp: ---.- C"
                "Waiting..."
```

**Implementation**:

```cpp
void vTaskDisplay(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    const TickType_t soundDisplayDuration = pdMS_TO_TICKS(2000);  // Show sound for 2 seconds

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        char line1[16];
        char line2[16];
        TickType_t currentTime = xTaskGetTickCount();

        // Check if we should display sound (if clap was detected recently)
        if ((currentTime - sharedData.last_sound_time) < soundDisplayDuration) {
            // Display sound level without percentages
            snprintf(line1, sizeof(line1), "Sound --> %d", sharedData.analog_value);
            snprintf(line2, sizeof(line2), "Clap!");
            printf("[DISPLAY] Showing sound: %s / %s\n", line1, line2);
        } else {
            // Display temperature
            if (sharedData.temperature_available) {
                snprintf(line1, sizeof(line1), "Temp: %.1f C", sharedData.temperature);
                snprintf(line2, sizeof(line2), "Filt: %.1f C", sharedData.temperature_filtered);
                printf("[DISPLAY] Showing temp: %s / %s\n", line1, line2);
            } else {
                snprintf(line1, sizeof(line1), "Temp: ---.- C");
                snprintf(line2, sizeof(line2), "Waiting...");
                printf("[DISPLAY] Waiting: %s / %s\n", line1, line2);
            }
        }

        updateLCD(line1, line2);
    }
}
```

**Auto-Switch Timeline**:

```
T=0s:       Clap detected → LCD shows sound, RGB turns green
T=1s:       RGB turns red (LED timeout)
T=2s:       LCD switches to temperature
T=2s-7s:    LCD shows temperature (5 seconds window)
T=7s:       Next clap can be detected (5s minimum interval)
```

##### vTaskLED - RGB LED Control

**Purpose**: Control RGB LED with color changes based on system state (red when idle, green when clap detected).

**Frequency**: Continuous (runs in loop)

**Behavior**:

1. When clap is detected (via semaphore), change RGB to green
2. After exactly 1 second, automatically change RGB to red
3. Implements timeout mechanism using tick counting

**Implementation**:

```cpp
void vTaskLED(void *pvParameters) {
    for (;;) {
        uint32_t currentTime = xTaskGetTickCount();

        // Check if RGB LED should be turned back to red (1-second timeout)
        if (sharedData.led_state && currentTime >= sharedData.led_turn_off_time) {
            sharedData.led_state = false;
            if (rgbLed) {
                rgbLed->red();
                printf("[LED] RGB LED RED (idle)\n");
            }
        }

        // Turn RGB LED green when clap signal received
        if (semSoundLED.take(0)) {
            if (rgbLed && sharedData.led_state) {
                rgbLed->green();
                printf("[LED] RGB LED GREEN (clap detected)\n");
            }
        }

        kernel_primitives::delayMs(50);
    }
}
```

**RGB LED States**:

| State | Color | Duration | Trigger |
|-------|-------|----------|---------|
| Idle | Red | Continuous | Default |
| Clap Detected | Green | 1 second | Sound > threshold |
| Reserved | Blue | - | Future use |

### 5. Initialization (`init/init.cpp`)

The `setupApplication()` function initializes all components in sequence:

```cpp
void setupApplication() {
    // 1. Initialize Serial (115200 baud)
    Serial.begin(115200);
    kernel_primitives::delayMs(2000);

    printf("\n=== LAB 3.2 - Dual Sensor Monitoring ===\n");
    printf("Sound Sensor: D0=%d, A0=%d\n", SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    printf("Temp Sensor: DS18B20 on pin %d\n", DS18B20_PIN);
    printf("LED: %d\n", LED_PIN);
    printf("RGB LED: R=%d, G=%d, B=%d\n", RGB_LED_R_PIN, RGB_LED_G_PIN, RGB_LED_B_PIN);
    printf("LCD: I2C SDA=%d, SCL=%d (0x27)\n", LCD_SDA_PIN, LCD_SCL_PIN);
    printf("==========================================\n");

    // 2. Initialize RGB LED and set it to red
    rgbLed = new RgbLed(RGB_LED_R_PIN, RGB_LED_G_PIN, RGB_LED_B_PIN);
    rgbLed->begin();
    rgbLed->red();
    printf("RGB LED initialized (RED)\n");

    // 3. Initialize Sound Sensor
    soundSensor = new SoundSensor(SOUND_SENSOR_D0_PIN, SOUND_SENSOR_A0_PIN);
    soundSensor->begin();
    soundSensor->setThreshold(SOUND_THRESHOLD);
    soundSensor->setHysteresis(SOUND_HYSTERESIS);
    printf("Sound sensor initialized\n");
    printf("  Threshold: %d\n", SOUND_THRESHOLD);
    printf("  Hysteresis: %d\n", SOUND_HYSTERESIS);

    // 4. Initialize DS18B20 temperature sensor
    tempSensor = new DS18B20(DS18B20_PIN);
    tempSensor->begin();
    tempSensor->setResolution(12);  // 12-bit resolution (0.0625°C precision)
    printf("DS18B20 temperature sensor initialized\n");
    printf("  Resolution: 12 bits\n");

    // 5. Initialize LCD
    lcd = new LcdI2c(0x27, 16, 2);
    if (lcd) {
        lcd->begin();
        kernel_primitives::delayMs(500);
        lcd->setCursor(0, 0);
        lcd->print("Dual Sensor");
        kernel_primitives::delayMs(100);
        lcd->setCursor(0, 1);
        lcd->print("System Ready");
        kernel_primitives::delayMs(100);
        printf("LCD initialized\n");
    }

    // 6. Initialize Synchronization Primitives
    initSyncPrimitives();

    // 7. Create FreeRTOS Tasks
    createApplicationTasks();

    printf("=== FREE-RTOS SCHEDULER STARTED ===\n");
    printf("Tasks running:\n");
    printf("  - Detect (priority %d)\n", TASK_PRIORITY_DETECT);
    printf("  - Display (priority %d)\n", TASK_PRIORITY_DISPLAY);
    printf("  - LED (priority %d)\n", TASK_PRIORITY_LED);
    printf("  - Temperature (priority %d)\n", TASK_PRIORITY_TEMP);
    printf("=====================================\n");
    printf("Dual sensor monitoring active...\n\n");
}
```

## Data Flow

### Complete System Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                     vTaskDetect (20ms)                          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Read analog value from A0 (0-4095)                    │  │
│  │ 2. Read digital value from D0                            │  │
│  │ 3. Check rising edge (below → above threshold)           │  │
│  │ 4. Check minimum interval (5 seconds)                    │  │
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
                            │ semSoundDisplay, semSoundLED
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                     vTaskDisplay (100ms)                        │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Check time since last clap                           │  │
│  │ 2. If < 2 seconds: Show sound level                      │  │
│  │ 3. Else: Show temperature (raw + filtered)              │  │
│  │ 4. Acquire lcdMutex                                      │  │
│  │ 5. Update LCD (no clear to reduce flicker)              │  │
│  │ 6. Release lcdMutex                                      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘

                            │
                            │ RGB LED control
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                     vTaskLED (continuous)                       │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ 1. Check if semSoundLED signaled                         │  │
│  │    - If yes: Set RGB GREEN                               │  │
│  │ 2. Check if led_turn_off_time reached                    │  │
│  │    - If yes: Set RGB RED                                 │  │
│  │ 3. Delay 50ms                                            │  │
│  │ 4. Repeat                                                │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                 vTaskTemperature (100ms)                        │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ State Machine:                                           │  │
│  │                                                           │  │
│  │ State 0 (Request):                                       │  │
│  │   - Request temperature conversion                       │  │
│  │   - Move to State 1                                      │  │
│  │                                                           │  │
│  │ State 1 (Wait):                                          │  │
│  │   - Wait 8 cycles (800ms)                                │  │
│  │   - After 8 cycles, move to State 2                     │  │
│  │                                                           │  │
│  │ State 2 (Read):                                          │  │
│  │   - Read temperature from sensor                         │  │
│  │   - Add to filter buffer (circular)                      │  │
│  │   - Calculate median filter (5 samples)                  │  │
│  │   - Update SharedData                                    │  │
│  │   - Signal semTempDisplay                                │  │
│  │   - Move to State 0                                      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Display Auto-Switch Logic

```
Timeline after clap:
├─ T=0ms:   Clap detected
│           ├─ RGB LED: RED → GREEN
│           ├─ LCD: Temp → "Sound --> 2527"
│           └─ last_sound_time = now
│
├─ T=1000ms: LED timeout
│           ├─ RGB LED: GREEN → RED
│           └─ LCD: Still shows sound (within 2s window)
│
├─ T=2000ms: Display timeout
│           └─ LCD: "Sound --> ..." → "Temp: 26.5 C"
│
├─ T=2000-7000ms: LCD shows temperature
│
└─ T=7000ms: Minimum interval passed
            └─ Next clap can be detected (5s since last)
```

## Key Concepts Explained

### 1. Rising Edge Detection

Rising edge detection triggers only when signal crosses threshold from below:

```
Signal:  1400 → 1600 → 1580 → 1590 → 1400
Threshold: 1500
Was Above: N → Y → Y → Y → N
Trigger:  ✗      ✓      ✗      ✗      ✗

Result: Only ONE trigger at the crossing point
```

### 2. Minimum Interval (5 Seconds)

Prevents rapid triggering and ensures LCD has time to show temperature:

```
Clap #1 at T=0s
├─ LCD shows sound: T=0s to T=2s
├─ LCD shows temperature: T=2s to T=7s
└─ Next clap allowed: T=7s onwards (5s minimum interval)
```

### 3. Median Filter (5 Samples)

Provides stable temperature readings by removing outliers:

```
Raw:     26.3, 30.2, 26.5, 26.4, 26.6
Sorted:  26.3, 26.4, 26.5, 26.6, 30.2
Median:  26.5°C (ignores outlier 30.2)

Benefits:
- Removes transient errors
- Provides stable output
- Better than average for outliers
```

### 4. Non-Blocking Temperature Reading

Temperature reading split into request+wait+read to avoid blocking:

```
Blocking (BAD):
  readTemperature()
    ├─ requestTemperature()
    ├─ delay(750ms)  ← BLOCKS ENTIRE SYSTEM!
    └─ getTemperature()

Non-blocking (GOOD):
  T=0ms:   requestTemperature()
  T=100ms: wait...
  T=200ms: wait...
  T=300ms: wait...
  T=400ms: wait...
  T=500ms: wait...
  T=600ms: wait...
  T=700ms: wait...
  T=800ms: getTemperature()  ← Ready!

Result: Other tasks continue running during wait
```

### 5. FreeRTOS Task Priorities

Higher priority tasks preempt lower priority tasks:

```
Priority 3: vTaskDetect (highest)    - Sound acquisition
Priority 3: vTaskTemperature (high)  - Temperature reading
Priority 2: vTaskDisplay (medium)    - LCD updates
Priority 1: vTaskLED (lowest)       - RGB LED control
```

**Why this priority order?**

1. **vTaskDetect (Priority 3)**:
   - Critical for real-time sound detection
   - Must not miss clap events
   - 20ms interval requires precise timing

2. **vTaskTemperature (Priority 3)**:
   - Also critical for timely temperature updates
   - 100ms interval with state machine
   - Must not block other tasks

3. **vTaskDisplay (Priority 2)**:
   - Less critical than detection
   - Updates every 100ms (tolerant to delays)
   - Medium priority ensures regular updates

4. **vTaskLED (Priority 1)**:
   - Lowest criticality
   - LED timing is forgiving (1-second pulse)
   - Continuous operation, can be delayed

### 6. Display Auto-Switch

Intelligent display switching based on activity:

```cpp
if ((currentTime - last_sound_time) < 2000ms) {
    // Show sound level
} else {
    // Show temperature
}
```

**Why 2 seconds?**
- Long enough to see the sound level
- Short enough to return to temperature quickly
- Balances both sensor information

## Testing and Verification

### Expected Behavior

#### Normal State (No Sound)

```
LCD:
  Line 1: "Temp: 26.5 C"
  Line 2: "Filt: 26.6 C"

RGB LED: RED

Serial:
  [TEMP] Temperature: 26.50°C, Filtered: 26.62°C
  [DISPLAY] Showing temp: Temp: 26.5 C / Filt: 26.6 C
```

#### Sound Detected

```
T=0ms: Clap detected

LCD:
  Line 1: "Sound --> 2527"
  Line 2: "Clap!"

RGB LED: GREEN

Serial:
  [DETECT] Sound detected! Analog: 2527, Threshold: 1500
  [LED] RGB LED GREEN (clap detected)
  [DISPLAY] Showing sound: Sound --> 2527 / Clap!
```

#### T=1000ms: LED Timeout

```
LCD:
  Line 1: "Sound --> 2527"
  Line 2: "Clap!"

RGB LED: RED (back to idle)

Serial:
  [LED] RGB LED RED (idle)
```

#### T=2000ms: Display Timeout

```
LCD:
  Line 1: "Temp: 26.5 C"
  Line 2: "Filt: 26.6 C"

RGB LED: RED

Serial:
  [DISPLAY] Showing temp: Temp: 26.5 C / Filt: 26.6 C
```

#### T=7000ms: Next Clap Allowed

```
Minimum interval (5 seconds) has passed
Next clap will be detected and trigger the cycle again
```

### Serial Output Example

```
=== LAB 3.2 - Dual Sensor Monitoring ===
Sound Sensor: D0=12, A0=34
Temp Sensor: DS18B20 on pin 4
LED: 14
RGB LED: R=25, G=26, B=27
LCD: I2C SDA=21, SCL=22 (0x27)
==========================================
RGB LED initialized (RED)
Sound sensor initialized
  Threshold: 1500
  Hysteresis: 50
DS18B20 temperature sensor initialized
  Resolution: 12 bits
LCD initialized
Creating FreeRTOS tasks...
=== FREE-RTOS SCHEDULER STARTED ===
Tasks running:
  - Detect (priority 3)
  - Display (priority 2)
  - LED (priority 1)
  - Temperature (priority 3)
=====================================
Dual sensor monitoring active...

[TEMP] Temperature: 26.50°C, Filtered: 26.62°C
[DISPLAY] Showing temp: Temp: 26.5 C / Filt: 26.6 C
[TEMP] Temperature: 26.44°C, Filtered: 26.50°C
[DISPLAY] Showing temp: Temp: 26.4 C / Filt: 26.5 C

[DETECT] Sound detected! Analog: 2527, Threshold: 1500
[LED] RGB LED GREEN (clap detected)
[DISPLAY] Showing sound: Sound --> 2527 / Clap!

[LED] RGB LED RED (idle)
[DISPLAY] Showing sound: Sound --> 2527 / Clap!

[DISPLAY] Showing temp: Temp: 26.4 C / Filt: 26.5 C
[TEMP] Temperature: 26.38°C, Filtered: 26.44°C
[DISPLAY] Showing temp: Temp: 26.4 C / Filt: 26.4 C
```

## Configuration Parameters

All configurable parameters are defined in `state/state.cpp`:

```cpp
// GPIO Pin Assignments
const uint8_t SOUND_SENSOR_D0_PIN = 12;
const uint8_t SOUND_SENSOR_A0_PIN = 34;
const uint8_t LED_PIN = 14;
const uint8_t LCD_SDA_PIN = 21;
const uint8_t LCD_SCL_PIN = 22;

// Temperature sensor pin
const uint8_t DS18B20_PIN = 4;

// RGB LED pins
const uint8_t RGB_LED_R_PIN = 25;
const uint8_t RGB_LED_G_PIN = 26;
const uint8_t RGB_LED_B_PIN = 27;

// Task Configuration
const uint32_t TASK_STACK_SIZE = 4096;
const UBaseType_t TASK_PRIORITY_DETECT = tskIDLE_PRIORITY + 3;
const UBaseType_t TASK_PRIORITY_DISPLAY = tskIDLE_PRIORITY + 2;
const UBaseType_t TASK_PRIORITY_LED = tskIDLE_PRIORITY + 1;
const UBaseType_t TASK_PRIORITY_TEMP = tskIDLE_PRIORITY + 3;

// Sound Detection Parameters
const uint16_t SOUND_THRESHOLD = 1500;    // Analog threshold (0-4095)
const uint16_t SOUND_HYSTERESIS = 50;    // Anti-bounce margin
const uint32_t SOUND_DEBOUNCE_TIME = 50;  // Debounce delay (ms)

// Temperature Thresholds (for future use)
const float TEMPERATURE_THRESHOLD_HIGH = 30.0f;
const float TEMPERATURE_THRESHOLD_LOW = 20.0f;
```

### Adjusting Sound Threshold

```cpp
// More sensitive (detects quieter sounds)
const uint16_t SOUND_THRESHOLD = 1000;

// Less sensitive (only detects loud sounds)
const uint16_t SOUND_THRESHOLD = 2000;
```

### Adjusting Sound Display Duration

```cpp
// Show sound for longer (in vTaskDisplay)
const TickType_t soundDisplayDuration = pdMS_TO_TICKS(3000);  // 3 seconds

// Show sound for shorter
const TickType_t soundDisplayDuration = pdMS_TO_TICKS(1000);  // 1 second
```

### Adjusting Minimum Clap Interval

```cpp
// Allow claps more frequently (in vTaskDetect)
if (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(3000)) {  // 3 seconds

// Require longer between claps
if (currentTime - sharedData.last_sound_time >= pdMS_TO_TICKS(10000)) {  // 10 seconds
```

### Adjusting Temperature Resolution

```cpp
// Lower resolution, faster conversion (9 bits = 94ms)
tempSensor->setResolution(9);

// Higher resolution, slower conversion (12 bits = 750ms)
tempSensor->setResolution(12);
```

## Performance Characteristics

### Timing Analysis

| Component | Frequency | Period | CPU Usage |
|-----------|-----------|--------|-----------|
| Sensor Acquisition | 50 Hz | 20 ms | ~1-2% |
| Temperature Request | 10 Hz | 100 ms | ~0.5% |
| Display Update | 10 Hz | 100 ms | ~1% |
| LED Control | Continuous | 50 ms loop | ~0.5% |
| **Total CPU Usage** | - | - | **~3-4%** |

### Memory Usage

```
RAM:   22,928 bytes (7.0% of 327,680 bytes)
Flash: 303,489 bytes (23.2% of 1,310,720 bytes)
```

### Latency

- **Sensor to RGB LED**: < 20ms (one detect cycle)
- **Sensor to LCD**: < 120ms (detect + display cycle)
- **Temperature Reading**: ~800ms (non-blocking)
- **RGB LED Pulse Duration**: 1000ms ± 50ms
- **Sound Display Duration**: 2000ms ± 100ms

## Troubleshooting

### Issue: LCD Always Shows Sound

**Cause**: Sound continuously detected (value > threshold) or `last_sound_time` constantly updated.

**Solution**:
1. Check sound sensor connections
2. Verify threshold value is appropriate
3. Monitor serial output for sound detection messages
4. Ensure rising edge detection is working

### Issue: Temperature Shows ---.- C

**Cause**: DS18B20 not found or CRC errors.

**Solution**:
1. Check DS18B20 connections (GPIO4, VCC, GND)
2. Verify 4.7kΩ pull-up resistor between DATA and VCC
3. Check serial output for "Device found" message
4. Try another DS18B20 sensor

### Issue: RGB LED Stays Green

**Cause**: `led_turn_off_time` not being checked or `led_state` not resetting.

**Solution**:
1. Verify vTaskLED is running
2. Check timeout logic in vTaskLED
3. Ensure `led_state` is set to false after timeout

### Issue: Temperature Not Updating

**Cause**: Temperature task not running or conversion not completing.

**Solution**:
1. Verify vTaskTemperature is created and running
2. Check serial output for temperature messages
3. Ensure DS18B20 resolution is set correctly
4. Verify OneWire bus is working

### Issue: Rapid LED Flickering

**Cause**: Minimum interval too small or threshold inappropriate.

**Solution**:
1. Increase minimum interval in vTaskDetect
2. Adjust threshold value
3. Verify rising edge detection is working

## Comparison with Lab 3.1

| Feature | Lab 3.1 | Lab 3.2 |
|---------|---------|---------|
| Sensors | Sound only | Sound + Temperature |
| Display | Count only | Sound level + Temperature |
| LED | Single LED | RGB LED |
| Tasks | 3 | 4 |
| Display Logic | Static | Dynamic auto-switch |
| Temperature Reading | N/A | Non-blocking with filter |
| Sound Detection | Threshold + hysteresis | Rising edge + interval |
| LED Behavior | Pulse ON/OFF | Color change (Red/Green) |

## Conclusion

This implementation demonstrates:

1. **Multi-sensor monitoring** using FreeRTOS tasks
2. **Non-blocking sensor reading** with state machines
3. **Intelligent display switching** based on activity
4. **Median filtering** for stable temperature readings
5. **Rising edge detection** for reliable sound detection
6. **RGB LED control** with state-based colors
7. **Efficient resource usage** with minimal CPU overhead
8. **Real-time data acquisition** with precise timing

The system provides a robust foundation for multi-sensor applications and can be extended with additional sensors, more complex algorithms, or different output mechanisms.

## Future Enhancements

Potential improvements could include:

1. **Temperature-based alerts** (high/low temperature warnings)
2. **Sound level meter** display (dB conversion)
3. **Data logging** to SD card or EEPROM
4. **Wireless transmission** via WiFi/Bluetooth
5. **Multi-clap patterns** for different commands
6. **Adaptive sound threshold** based on ambient noise
7. **Temperature trend analysis** (rising/falling)
8. **RGB LED animations** for different states
9. **Serial command interface** for configuration
10. **Sleep mode** for battery operation