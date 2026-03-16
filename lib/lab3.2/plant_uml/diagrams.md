# Dual Sensor System - UML Diagrams

## 1. Architecture Diagram

```mermaid
graph TB
    subgraph Hardware["Hardware Components"]
        Sound[Sound Sensor<br/>D0:12, A0:34]
        Temp[DS18B20<br/>Temp Sensor<br/>Pin:4]
        RGB[RGB LED<br/>R:25, G:26, B:27]
        LCD[LCD<br/>I2C:21/22]
    end

    subgraph Tasks["FreeRTOS Tasks"]
        Detect[vTaskDetect<br/>20ms, P3]
        TempTask[vTaskTemperature<br/>100ms, P3]
        Display[vTaskDisplay<br/>100ms, P2]
        LEDTask[vTaskLED<br/>Continuous, P1]
    end

    subgraph Sync["Synchronization"]
        Sem[Semaphores]
        Mutex[Mutex]
    end

    Sound -->|reads| Detect
    Temp -->|reads| TempTask
    Detect -->|signals| Sem
    TempTask -->|signals| Sem
    Sem -->|triggers| Display
    Sem -->|triggers| LEDTask
    Display -->|locks| Mutex
    Display -->|updates| LCD
    LEDTask -->|controls| RGB

    note1{Thresh: 1500<br/>Min Interval: 5s<br/>Temp: 12-bit<br/>Filter: Median}
```

## 2. Component Diagram

```mermaid
graph TB
    subgraph HW["Hardware"]
        SoundSensor[SoundSensor<br/>D0:12, A0:34]
        TempSensor[DS18B20<br/>Pin:4]
        RGBLed[RGB LED<br/>R:25, G:26, B:27]
        LCDComp[LCD<br/>I2C:21/22]
    end

    subgraph App["FreeRTOS App"]
        DetectComp[Detect]
        TempComp[Temperature]
        DisplayComp[Display]
        LEDComp[LED]
        SharedData[SharedData]
    end

    subgraph SyncComp["Sync"]
        SyncMutex[Mutex]
        Semaphore[Semaphore<br/>3 semaphores]
    end

    DetectComp --> SoundSensor
    DetectComp --> SharedData
    TempComp --> TempSensor
    TempComp --> SharedData
    DisplayComp --> LCDComp
    DisplayComp --> SyncMutex
    LEDComp --> RGBLed
    LEDComp --> Semaphore

    note1{SharedData:<br/>sound_count<br/>temp<br/>led_state<br/>timestamps}
```

## 3. Class Diagram

```mermaid
classDiagram
    class SoundSensor {
        +readDigital() bool
        +readAnalog() uint16
        +isSoundDetected() bool
        +setThreshold(uint16) void
        +setHysteresis(uint16) void
    }

    class DS18B20 {
        +requestTemperature() bool
        +getTemperature() float
        +setResolution(uint8) void
        +isPresent() bool
    }

    class RgbLed {
        +red() void
        +green() void
        +blue() void
        +off() void
    }

    class LcdI2c {
        +print(text) void
        +clear() void
        +setCursor(col, row) void
    }

    class SharedData {
        +sound_count uint32
        +temperature float
        +temperature_filtered float
        +led_state bool
        +last_sound_time uint32
        +analog_value uint16
    }

    class vTaskDetect {
        +main() void
    }

    class vTaskTemperature {
        +main() void
    }

    class vTaskDisplay {
        +main() void
    }

    class vTaskLED {
        +main() void
    }

    vTaskDetect --> SoundSensor : uses
    vTaskDetect --> SharedData : uses
    vTaskTemperature --> DS18B20 : uses
    vTaskTemperature --> SharedData : uses
    vTaskDisplay --> LcdI2c : uses
    vTaskDisplay --> SharedData : uses
    vTaskLED --> RgbLed : uses
    vTaskLED --> SharedData : uses
```

## 4. Detect Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> Wait[Wait 20ms]
    Wait --> Read[Read sensors<br/>Analog + Digital]
    Read --> Edge{rising<br/>edge?}

    Edge -->|yes| Interval{5s<br/>elapsed?}

    Interval -->|yes| Increment[Increment count]
    Increment --> UpdateTime[Update timestamps]
    UpdateTime --> SignalLED[Signal LED]
    SignalLED --> SignalDisplay[Signal Display]
    SignalDisplay --> ResetFlag[Reset edge flag]
    ResetFlag --> Loop

    Interval -->|no| ResetFlag
    Edge -->|no| Loop((Loop back))
    Loop --> Wait
```

## 5. Temperature Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> Wait[Wait 100ms]
    Wait --> State{state?}

    State -->|Not requested| Request[Request conversion]
    Request --> SetState1[Set state=Wait<br/>Reset cycles]
    SetState1 --> Loop((Loop back))

    State -->|Wait| CheckCycles{8 cycles<br/>elapsed?}
    CheckCycles -->|no| Increment[Increment cycles]
    Increment --> Loop

    CheckCycles -->|yes| Read[Read temperature]
    Read --> Filter[Apply median filter]
    Filter --> Update[Update SharedData]
    Update --> SignalTemp[Signal Display]
    SignalTemp --> SetState0[Set state=Request]
    SetState0 --> Loop
```

## 6. Display Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> Wait[Wait 100ms]
    Wait --> CheckTime{Time since<br/>last clap}

    CheckTime -->|< 2s| FormatSound[Format sound<br/>Sound --> XXXX]
    FormatSound --> Lock

    CheckTime -->|>= 2s| CheckTemp{temp<br/>available?}

    CheckTemp -->|yes| FormatTemp[Format temp<br/>Temp: XX.X C]
    FormatTemp --> Lock

    CheckTemp -->|no| FormatWait[Format waiting<br/>Temp: ---.- C]
    FormatWait --> Lock

    Lock[Lock mutex]
    Lock --> Update[Update LCD<br/>No clear]
    Update --> Release[Release mutex]
    Release --> Loop((Loop back))
    Loop --> Wait
```

## 7. LED Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> TimeoutCheck{timeout<br/>reached?}
    TimeoutCheck -->|yes| TurnRED[Turn RGB RED]
    TurnRED --> SemaphoreCheck

    TimeoutCheck -->|no| SemaphoreCheck

    SemaphoreCheck{semaphore<br/>signaled?}
    SemaphoreCheck -->|yes| TurnGREEN[Turn RGB GREEN]
    TurnGREEN --> Delay[Delay 50ms]

    SemaphoreCheck -->|no| Delay
    Delay --> Loop((Loop back))
    Loop --> TimeoutCheck
```

## 8. Data Flow Diagram

```mermaid
graph LR
    Sensor[Sound Sensor] -->|analog/digital| Detect[Detect]
    Detect -->|count/state| SharedData[SharedData]
    TempSensor[DS18B20] -->|temperature| TempTask[Temperature]
    TempTask -->|temp/filtered| SharedData
    SharedData -->|last_sound_time| Display[Display]
    SharedData -->|temp| Display
    Display -->|Sound/Temp| LCD[LCD]
    SharedData -->|led_state| LED[LED]
    LED -->|Red/Green| RGB[RGB HW]

    note1{Thresh: 1500<br/>Filter: Median}
```

## 9. Software Layers Diagram

```mermaid
graph TB
    subgraph Application["Application Layer"]
        AppTasks[Detect, Temperature,<br/>Display, LED]
    end

    subgraph HAL["Hardware Abstraction Layer"]
        HWSound[Sound Sensor]
        HWTemp[DS18B20]
        HWRGB[RGB LED]
        HWLCD[LCD]
    end

    subgraph FreeRTOS["FreeRTOS Kernel"]
        RTOSTasks[Task]
        RTOSMutex[Mutex]
        RTOSSem[Semaphore]
    end

    Application -->|uses| HAL
    Application -->|creates| FreeRTOS

    note{4 Tasks:<br/>20ms, 100ms,<br/>100ms, Continuous}
```

## 10. Detailed Low-Level Architecture

```mermaid
graph TB
    ESP32_Hardware[ESP32 Hardware<br/>ADC0: Pin 34<br/>GPIO12: Digital<br/>GPIO4: OneWire<br/>RGB: 25,26,27<br/>I2C: SDA21/SCL22]

    HAL[HAL Layer<br/>SoundSensor<br/>_digitalPin: 12<br/>_analogPin: 34<br/>_threshold: 1500<br/>_hysteresis: 50<br/><br/>DS18B20<br/>_pin: 4<br/>_resolution: 12<br/>_address: 8 bytes<br/>_deviceFound: bool<br/><br/>RgbLed<br/>_redPin: 25<br/>_greenPin: 26<br/>_bluePin: 27<br/><br/>LcdI2c<br/>_address: 0x27<br/>_cols: 16<br/>_rows: 2]

    Tasks[FreeRTOS Tasks<br/>vTaskDetect: 20ms P3<br/>Stack: 4096<br/><br/>vTaskTemperature: 100ms P3<br/>Stack: 4096<br/><br/>vTaskDisplay: 100ms P2<br/>Stack: 4096<br/><br/>vTaskLED: Continuous P1<br/>Stack: 4096]

    Sync[Sync Primitives<br/>lcdMutex: Mutex<br/>Timeout: 100ms<br/>Protects: LCD I2C<br/><br/>semSoundDisplay: Binary<br/>Trigger: Display<br/><br/>semSoundLED: Binary<br/>Trigger: LED<br/><br/>semTempDisplay: Binary<br/>Trigger: Display]

    Data[SharedData<br/>uint16_t analog_value<br/>uint32_t sound_count<br/>float temperature<br/>float temperature_filtered<br/>bool led_state<br/>uint32_t led_turn_off_time<br/>uint32_t last_sound_time<br/>uint32_t last_temperature_time<br/>float temperature_buffer[5]<br/>uint8_t temperature_buffer_index]

    Config[Configuration<br/>SOUND_THRESHOLD: 1500<br/>SOUND_HYSTERESIS: 50<br/>MIN_INTERVAL: 5000ms<br/>SOUND_DISPLAY: 2000ms<br/>TEMP_RESOLUTION: 12bits<br/>FILTER_SIZE: 5 samples]

    ESP32_Hardware -->|GPIO/ADC/OneWire| HAL
    HAL -->|methods| Tasks
    Tasks -->|uses| Sync
    Tasks -->|updates| Data
    Sync -->|protects| Data
    Config -->|used by| Tasks
```

## 11. Sound Sensor Sequence Diagram

```mermaid
sequenceDiagram
    participant T as vTaskDetect
    participant SS as SoundSensor
    participant HW as ESP32 GPIO/ADC
    participant SD as SharedData

    Note over T,HW: Every 20ms cycle

    T->>SS: readDigital()
    SS->>HW: digitalRead(PIN 12)
    HW-->>SS: HIGH/LOW (bool)
    SS-->>T: HIGH/LOW (bool)

    T->>SS: readAnalog()
    SS->>HW: analogRead(PIN 34)
    HW-->>SS: 0-4095 (uint16)
    SS-->>T: 0-4095 (uint16)

    alt Rising edge detected
        T->>T: Check 5s interval
        T-->>T: elapsed (bool)

        T->>SD: sound_count++
        SD-->>T: new count (uint32)

        T->>SD: led_state = true
        SD-->>T: confirmed (bool)

        T->>SD: led_turn_off_time = now + 1000ms
        SD-->>T: timestamp (uint32)

        T->>SD: last_sound_time = now
        SD-->>T: timestamp (uint32)

        T->>SD: analog_value = current_value
        SD-->>T: stored (bool)

        T->>T: give(semSoundDisplay)
        T-->>T: success (bool)

        T->>T: give(semSoundLED)
        T-->>T: success (bool)

        T->>T: Set wasAboveThreshold = true
    else Below threshold
        T->>T: Set wasAboveThreshold = false
    end

    Note over T,HW: Wait until next 20ms period
```

## 12. Temperature Sensor Sequence Diagram

```mermaid
sequenceDiagram
    participant T as vTaskTemperature
    participant DS as DS18B20
    participant OW as OneWire Bus
    participant SD as SharedData

    Note over T,SD: Every 100ms cycle

    alt State 0: Request conversion
        T->>DS: requestTemperature()
        activate DS
        DS->>OW: reset()
        OW-->>DS: presence (bool)
        DS->>OW: select(address)
        OW-->>DS: selected (bool)
        DS->>OW: write(0x44)
        OW-->>DS: written (bool)
        DS-->>T: success (bool)
        deactivate DS

        T->>T: Set conversionRequested = true
        T->>T: Reset waitCycles = 0
    else State 1: Wait for conversion
        T->>T: waitCycles++
        alt waitCycles >= 8 (800ms)
            T->>T: Move to read state
        else Continue waiting
        end
    else State 2: Read temperature
        T->>DS: getTemperature()
        activate DS
        DS->>OW: reset()
        OW-->>DS: presence (bool)
        DS->>OW: select(address)
        OW-->>DS: selected (bool)
        DS->>OW: write(0xBE)
        OW-->>DS: written (bool)

        loop Read 9 bytes
            DS->>OW: read()
            OW-->>DS: byte (uint8)
        end

        DS->>DS: Verify CRC
        DS->>DS: Convert to Celsius
        DS-->>T: temperature (float)
        deactivate DS

        T->>SD: temperature = temp
        SD-->>T: stored (bool)

        T->>SD: Add to filter buffer
        SD-->>T: stored (bool)

        T->>T: Calculate median filter
        T-->>T: filtered (float)

        T->>SD: temperature_filtered = filtered
        SD-->>T: stored (bool)

        T->>T: give(semTempDisplay)
        T-->>T: success (bool)

        T->>T: Set conversionRequested = false
    end

    Note over T,SD: Wait until next 100ms period
```

## 13. LCD Sequence Diagram

```mermaid
sequenceDiagram
    participant D as vTaskDisplay
    participant SD as SharedData
    participant M as lcdMutex
    participant LCD as LcdI2c
    participant I2C as I2C Bus

    Note over D,I2C: Every 100ms cycle

    D->>D: Wait 100ms
    D-->>D: elapsed (bool)

    D->>SD: Check last_sound_time
    SD-->>D: timestamp (uint32)

    alt < 2 seconds since clap
        D->>D: Format sound string<br/>"Sound --> %d"
        D-->>D: formatted (char*)
    else >= 2 seconds
        D->>SD: Check temperature_available
        SD-->>D: available (bool)

        alt Temperature available
            D->>SD: Read temperature
            SD-->>D: temp (float)

            D->>SD: Read temperature_filtered
            SD-->>D: filtered (float)

            D->>D: Format temp strings<br/>"Temp: %.1f C"<br/>"Filt: %.1f C"
            D-->>D: formatted (char*)
        else Waiting for temp
            D->>D: Format waiting<br/>"Temp: ---.- C"
            D-->>D: formatted (char*)
        end
    end

    D->>M: take(100ms)
    activate M
    M-->>D: true (success)
    deactivate M

    D->>LCD: setCursor(0, 0)
    activate LCD
    LCD->>I2C: I2C write 0x80
    I2C-->>LCD: ACK
    LCD-->>D: positioned (void)
    deactivate LCD

    D->>LCD: print(line1)
    activate LCD
    loop Each character
        LCD->>I2C: I2C write character
        I2C-->>LCD: ACK
    end
    LCD-->>D: written (int)
    deactivate LCD

    D->>LCD: setCursor(0, 1)
    activate LCD
    LCD->>I2C: I2C write 0xC0
    I2C-->>LCD: ACK
    LCD-->>D: positioned (void)
    deactivate LCD

    D->>LCD: print(line2)
    activate LCD
    loop Each character
        LCD->>I2C: I2C write character
        I2C-->>LCD: ACK
    end
    LCD-->>D: written (int)
    deactivate LCD

    D->>M: give()
    activate M
    M-->>D: released (bool)
    deactivate M

    Note over D,I2C: Wait until next 100ms period
```

## 14. RGB LED Sequence Diagram

```mermaid
sequenceDiagram
    participant L as vTaskLED
    participant SD as SharedData
    participant S as semSoundLED
    participant RGB as RgbLed
    participant HW as GPIO 25,26,27

    Note over L,HW: Continuous loop (every 50ms)

    L->>L: Delay 50ms
    L-->>L: elapsed (bool)

    alt Semaphore signaled
        L->>S: take(0ms)
        activate S
        S-->>L: true (signal received)
        deactivate S

        L->>SD: Read led_state
        SD-->>L: led_state (bool)

        alt LED should be ON (true)
            L->>RGB: green()
            activate RGB
            RGB->>HW: digitalWrite(25, LOW)
            HW-->>RGB: success
            RGB->>HW: digitalWrite(26, HIGH)
            HW-->>RGB: success
            RGB->>HW: digitalWrite(27, LOW)
            HW-->>RGB: success
            RGB-->>L: void
            deactivate RGB
        end
    end

    alt Timeout check (every 50ms)
        L->>SD: Read led_turn_off_time
        SD-->>L: timestamp (uint32)

        L->>L: currentTime = xTaskGetTickCount()
        L-->>L: current ticks

        alt currentTime >= led_turn_off_time
            L->>SD: led_state = false
            SD-->>L: confirmed (bool)

            L->>RGB: red()
            activate RGB
            RGB->>HW: digitalWrite(25, HIGH)
            HW-->>RGB: success
            RGB->>HW: digitalWrite(26, LOW)
            HW-->>RGB: success
            RGB->>HW: digitalWrite(27, LOW)
            HW-->>RGB: success
            RGB-->>L: void
            deactivate RGB
        else Keep current state
            L->>SD: Read led_state
            SD-->>L: current state (bool)
        end
    end

    Note over L,HW: Loop continues every 50ms
```

## 15. Data Flow - Sound Input Layer

```mermaid
graph LR
    subgraph Hardware_Input["Hardware Input Layer"]
        ADC[ADC0<br/>Pin: 34<br/>Function: Read analog<br/>Returns: 0-4095<br/>ESP32 ADC1_CH6]

        DIG[GPIO 12<br/>Function: Read digital<br/>Returns: HIGH/LOW<br/>Sound Sensor D0]

        SOUND[Sound Sensor HW<br/>Function: Detect sound<br/>Output: Digital/Analog<br/>Threshold: Adjustable]
    end

    subgraph HAL_Input["HAL Input Layer"]
        Sensor[SoundSensor<br/>readDigital(): digitalRead 12<br/>readAnalog(): analogRead 34<br/>isSoundDetected(): edge detection<br/>_lastDigitalState: bool<br/>_currentValue: uint16<br/>_threshold: 1500<br/>_hysteresis: 50]
    end

    subgraph Task_Input["Task Input Layer"]
        Detect[vTaskDetect<br/>Function: Read sensors<br/>Period: 20ms<br/>Priority: 3<br/>Rising edge detection]
    end

    SOUND -->|analog signal| ADC
    SOUND -->|digital signal| DIG

    ADC -->|value 0-4095| Sensor
    DIG -->|HIGH/LOW| Sensor

    Sensor -->|bool HIGH/LOW| Detect
    Sensor -->|uint16 0-4095| Detect
    Sensor -->|bool soundDetected| Detect
```

## 16. Data Flow - Temperature Input Layer

```mermaid
graph LR
    subgraph Hardware_Temp["Hardware Temperature Layer"]
        ONEWIRE[OneWire Bus<br/>Pin: 4<br/>Function: Digital comm<br/>Protocol: 1-Wire<br/>Pull-up: 4.7kΩ]

        DS18B20[DS18B20 Sensor<br/>Function: Measure temp<br/>Range: -55°C to +125°C<br/>Resolution: 12-bit<br/>Power: 3.3V/5V]
    end

    subgraph HAL_Temp["HAL Temperature Layer"]
        Temp[DS18B20<br/>requestTemperature(): Start conv<br/>getTemperature(): Read result<br/>setResolution(): 9-12 bits<br/>_pin: 4<br/>_address: 8 bytes<br/>_resolution: 12<br/>_deviceFound: bool<br/>_oneWire: OneWire*]
    end

    subgraph Task_Temp["Task Temperature Layer"]
        TempTask[vTaskTemperature<br/>Function: Read temp<br/>Period: 100ms<br/>Priority: 3<br/>State machine: 3 states]
    end

    DS18B20 -->|digital 1-wire| ONEWIRE
    ONEWIRE -->|digital protocol| Temp

    Temp -->|request| ONEWIRE
    Temp -->|read| ONEWIRE

    Temp -->|temperature float| TempTask
    Temp -->|success bool| TempTask
```

## 17. Data Flow - Processing Layer

```mermaid
graph TB
    subgraph Detect_Task["vTaskDetect Processing"]
        Read[Read Sensors<br/>Function: Call SoundSensor methods<br/>Calls: readDigital, readAnalog<br/>Returns: bool, uint16]

        Edge[Rising Edge Detection<br/>Function: Check below→above<br/>Logic: value > 1500 && !wasAbove<br/>Returns: bool true/false]

        Interval[Minimum Interval Check<br/>Function: Check 5 seconds<br/>Logic: now - last >= 5000ms<br/>Purpose: Allow temp display]

        Update[Update SharedData<br/>Function: sound_count++<br/>led_state = true<br/>led_turn_off_time = now+1000ms<br/>last_sound_time = now]
    end

    subgraph Temp_Task["vTaskTemperature Processing"]
        Request[Request Conversion<br/>Function: Start temp conv<br/>Call: requestTemperature<br/>Sets: wait cycles = 0]

        WaitConversion[Wait for Conversion<br/>Function: Wait 8 cycles<br/>Logic: waitCycles < 8<br/>Duration: 800ms]

        ReadTemp[Read Temperature<br/>Function: getTemperature<br/>Reads: 9 bytes<br/>Returns: float]

        Filter[Median Filter<br/>Function: Filter 5 samples<br/>Logic: Sort and pick middle<br/>Removes: Outliers]
    end

    subgraph Constants["Configuration"]
        Config[Constants<br/>SOUND_THRESHOLD: 1500<br/>MIN_INTERVAL: 5000ms<br/>TEMP_RESOLUTION: 12<br/>FILTER_SIZE: 5<br/>LED_TIMEOUT: 1000ms]
    end

    Read --> Edge
    Edge --> Interval
    Interval --> Update

    Request --> WaitConversion
    WaitConversion --> ReadTemp
    ReadTemp --> Filter

    Config --> Edge
    Config --> Interval
    Config --> Request
    Config --> Filter
```

## 18. Data Flow - Storage Layer

```mermaid
graph TB
    subgraph Shared_Data["SharedData Structure"]
        Analog[analog_value<br/>Type: uint16<br/>Purpose: Store current ADC<br/>Writer: vTaskDetect<br/>Range: 0-4095]

        SoundCount[sound_count<br/>Type: uint32<br/>Purpose: Event counter<br/>Writer: vTaskDetect<br/>Reader: vTaskDisplay]

        LED_State[led_state<br/>Type: bool<br/>Purpose: RGB LED state<br/>Writer: vTaskDetect<br/>Reader: vTaskLED]

        LED_Time[led_turn_off_time<br/>Type: uint32<br/>Purpose: 1s timeout<br/>Writer: vTaskDetect<br/>Reader: vTaskLED]

        Last_Sound[last_sound_time<br/>Type: uint32<br/>Purpose: Last clap time<br/>Writer: vTaskDetect<br/>Reader: vTaskDisplay]

        Temperature[temperature<br/>Type: float<br/>Purpose: Raw temp reading<br/>Writer: vTaskTemperature<br/>Reader: vTaskDisplay]

        TempFiltered[temperature_filtered<br/>Type: float<br/>Purpose: Filtered temp<br/>Writer: vTaskTemperature<br/>Reader: vTaskDisplay]

        TempBuffer[temperature_buffer[5]<br/>Type: float array<br/>Purpose: Circular buffer<br/>Writer: vTaskTemperature<br/>Size: 5 samples]
    end

    subgraph Memory["Memory Access"]
        WriteDetect[Write: vTaskDetect<br/>Period: 20ms<br/>Priority: 3<br/>Writes: sound_count, led_state,<br/>last_sound_time, analog_value]

        WriteTemp[Write: vTaskTemperature<br/>Period: 800ms<br/>Priority: 3<br/>Writes: temperature,<br/>temperature_filtered,<br/>temperature_buffer]

        ReadDisplay[Read: vTaskDisplay<br/>Period: 100ms<br/>Priority: 2<br/>Reads: sound_count,<br/>temperature, temperature_filtered]

        ReadLED[Read: vTaskLED<br/>Continuous<br/>Priority: 1<br/>Reads: led_state,<br/>led_turn_off_time]
    end

    WriteDetect -->|store| Analog
    WriteDetect -->|increment| SoundCount
    WriteDetect -->|set| LED_State
    WriteDetect -->|set| LED_Time
    WriteDetect -->|set| Last_Sound

    WriteTemp -->|set| Temperature
    WriteTemp -->|set| TempFiltered
    WriteTemp -->|update| TempBuffer

    ReadDisplay -->|read| SoundCount
    ReadDisplay -->|read| Temperature
    ReadDisplay -->|read| TempFiltered
    ReadDisplay -->|read| Last_Sound

    ReadLED -->|read| LED_State
    ReadLED -->|read| LED_Time
```

## 19. Data Flow - Display Output

```mermaid
graph LR
    subgraph Display_Task["vTaskDisplay Output"]
        Wait[Wait 100ms<br/>Function: vTaskDelayUntil<br/>Period: 100ms<br/>Priority: 2]

        CheckTime[Check Time<br/>Function: Compare timestamps<br/>Logic: now - last_sound_time<br/>Duration: 2000ms]

        FormatSound[Format Sound<br/>Function: snprintf<br/>Format: Sound --> %d<br/>Buffer: 16 chars]

        FormatTemp[Format Temp<br/>Function: snprintf<br/>Format: Temp: %.1f C<br/>Buffer: 16 chars]

        Lock[Lock Mutex<br/>Function: lcdMutex.take<br/>Timeout: 100ms<br/>Purpose: Protect I2C]
    end

    subgraph LCD_Hardware["LCD Hardware"]
        I2C[I2C Bus<br/>SDA: Pin 21<br/>SCL: Pin 22<br/>Address: 0x27<br/>Speed: 100kHz]

        LCD[LCD Controller<br/>Type: PCF8574<br/>Size: 16x2 chars<br/>Function: Display text]
    end

    subgraph LCD_Ops["LCD Operations"]
        Cursor[Set Cursor<br/>Row 0: 0x80<br/>Row 1: 0xC0<br/>Function: lcd.setCursor]

        Write[Write Text<br/>Function: lcd.print<br/>Chars: ASCII<br/>Returns: void]

        NoClear[No Clear<br/>Optimization: Don't clear<br/>Purpose: Reduce flicker]
    end

    Wait --> CheckTime

    CheckTime -->|< 2000ms| FormatSound
    CheckTime -->|>= 2000ms| FormatTemp

    FormatSound --> Lock
    FormatTemp --> Lock

    Lock --> I2C
    I2C --> LCD

    LCD --> NoClear
    NoClear --> Cursor
    Cursor --> Write

    Unlock[Unlock Mutex<br/>Function: lcdMutex.give<br/>Release I2C<br/>Next task]

    Write --> Unlock
    Unlock --> I2C
```

## 20. Data Flow - RGB LED Output

```mermaid
graph LR
    subgraph LED_Task["vTaskLED Output"]
        Trigger[Trigger<br/>semSoundLED<br/>Binary Semaphore<br/>Signal from Detect]

        Loop[Loop<br/>Period: 50ms<br/>Priority: 1<br/>Continuous]

        CheckState[Check State<br/>Read led_state<br/>Read timestamp<br/>Compare current]

        Control[Control GPIO<br/>Pins: 25,26,27<br/>Type: Output<br/>Mode: Push-pull]
    end

    subgraph RGB_Hardware["RGB LED Hardware"]
        GPIO[GPIO 25,26,27<br/>ESP32 GPIO<br/>Direction: OUT<br/>Drive strength: High]

        RGB_HW[RGB LED HW<br/>Anode: GPIO 25,26,27<br/>Cathode: GND<br/>Type: Common Cathode<br/>Colors: R,G,B]

        State[RGB State<br/>RED: 100,0,0<br/>GREEN: 0,100,0<br/>Duration: 1000ms]
    end

    subgraph LED_Logic["LED Logic"]
        TurnRED[Turn RED<br/>digitalWrite 25 HIGH<br/>digitalWrite 26 LOW<br/>digitalWrite 27 LOW<br/>Set: led_state=false]

        TurnGREEN[Turn GREEN<br/>digitalWrite 25 LOW<br/>digitalWrite 26 HIGH<br/>digitalWrite 27 LOW<br/>Set: led_state=true]

        Timeout[Timeout Check<br/>led_turn_off_time<br/>Current time<br/>Compare: >=]
    end

    Trigger --> Loop
    Loop --> CheckState
    CheckState --> Control

    Control --> GPIO
    GPIO --> RGB_HW
    RGB_HW --> State

    State -->|signal received| TurnGREEN
    TurnGREEN --> GPIO

    State -->|timeout reached| Timeout
    Timeout --> TurnRED
    TurnRED --> GPIO

    Time[Timing<br/>ON duration: 1000ms<br/>Check interval: 50ms<br/>Priority: Low]

    Loop --> Time
```

## 21. Data Flow - Synchronization

```mermaid
graph TB
    subgraph Semaphores["Binary Semaphores"]
        Sem1[semSoundDisplay<br/>Type: Binary Semaphore<br/>Handle: SemaphoreHandle_t<br/>Creator: Detect<br/>Waiter: Display<br/>Trigger: Sound event]

        Sem2[semSoundLED<br/>Type: Binary Semaphore<br/>Handle: SemaphoreHandle_t<br/>Creator: Detect<br/>Waiter: LED<br/>Trigger: Sound event]

        Sem3[semTempDisplay<br/>Type: Binary Semaphore<br/>Handle: SemaphoreHandle_t<br/>Creator: Temperature<br/>Waiter: Display<br/>Trigger: Temp ready]
    end

    subgraph Mutex["Mutex Protection"]
        Mutex1[lcdMutex<br/>Type: Mutex<br/>Handle: SemaphoreHandle_t<br/>Protects: I2C LCD<br/>Timeout: 100ms<br/>Owner: Display]
    end

    subgraph Operations["Semaphore Operations"]
        Give[Give (signal)<br/>xSemaphoreGive<br/>Return: pdTRUE<br/>Unblocks task<br/>Non-blocking]

        Take[Take (wait)<br/>xSemaphoreTake<br/>Return: pdTRUE/FALSE<br/>Timeout: specified<br/>Blocking]
    end

    subgraph Flow["Signal Flow"]
        Detect[Detect Task<br/>Priority: 3<br/>Period: 20ms<br/>Signal giver]

        Temp[Temperature Task<br/>Priority: 3<br/>Period: 800ms<br/>Signal giver]

        Display[Display Task<br/>Priority: 2<br/>Period: 100ms<br/>Signal waiter<br/>Auto-switch logic]

        LED[LED Task<br/>Priority: 1<br/>Continuous<br/>Signal waiter<br/>Max wait: 0ms]
    end

    Detect -->|sound detected| Sem1
    Detect -->|sound detected| Sem2

    Temp -->|temp ready| Sem3

    Sem1 -->|trigger| Display
    Sem2 -->|trigger| LED
    Sem3 -->|trigger| Display

    Display -->|take| Sem1
    Display -->|take| Sem3
    LED -->|take| Sem2

    Sem1 -->|give| Detect
    Sem2 -->|give| Detect
    Sem3 -->|give| Temp

    Display -->|lock| Mutex1
    Mutex1 -->|unlock| Display

    Give --> Sem1
    Take --> Sem1
    Give --> Sem2
    Take --> Sem2
    Give --> Sem3
    Take --> Sem3

    Sync[Sync Pattern<br/>Detect: Signal (2 semaphores)<br/>Temp: Signal (1 semaphore)<br/>Display: Wait/Auto-switch<br/>LED: Wait/Control<br/>LCD: Lock/Write]

    Detect --> Sync
    Temp --> Sync
    Display --> Sync
    LED --> Sync
```

## 22. Display Auto-Switch Sequence Diagram

```mermaid
sequenceDiagram
    participant D as vTaskDisplay
    participant SD as SharedData
    participant M as lcdMutex
    participant LCD as LcdI2c
    participant I2C as I2C Bus

    Note over D,I2C: Display Auto-Switch Logic

    Note over D,SD: Initial state (idle)
    D->>SD: Check last_sound_time
    SD-->>D: timestamp

    D->>D: now - last_sound_time >= 2000ms
    D-->>D: true (show temp)

    D->>M: take(100ms)
    activate M
    M-->>D: true
    deactivate M

    D->>LCD: print("Temp: 26.5 C")
    activate LCD
    LCD->>I2C: I2C write
    I2C-->>LCD: ACK
    LCD-->>D: written
    deactivate LCD

    D->>LCD: print("Filt: 26.6 C")
    activate LCD
    LCD->>I2C: I2C write
    I2C-->>LCD: ACK
    LCD-->>D: written
    deactivate LCD

    D->>M: give()
    activate M
    M-->>D: released
    deactivate M

    Note over D,SD: Clap detected (by vTaskDetect)
    D->>SD: last_sound_time = now
    SD-->>D: updated

    Note over D,SD: Next display cycle (< 2s)
    D->>SD: Check last_sound_time
    SD-->>D: timestamp

    D->>D: now - last_sound_time < 2000ms
    D-->>D: true (show sound)

    D->>M: take(100ms)
    activate M
    M-->>D: true
    deactivate M

    D->>LCD: print("Sound --> 2527")
    activate LCD
    LCD->>I2C: I2C write
    I2C-->>LCD: ACK
    LCD-->>D: written
    deactivate LCD

    D->>LCD: print("Clap!")
    activate LCD
    LCD->>I2C: I2C write
    I2C-->>LCD: ACK
    LCD-->>D: written
    deactivate LCD

    D->>M: give()
    activate M
    M-->>D: released
    deactivate M

    Note over D,SD: 2 seconds elapsed
    D->>SD: Check last_sound_time
    SD-->>D: timestamp

    D->>D: now - last_sound_time >= 2000ms
    D-->>D: true (show temp)

    Note over D,SD: Returns to temperature display
```

## 23. Complete System Timeline

```mermaid
gantt
    title Dual Sensor System Timeline (Single Clap Event)
    dateFormat s
    axisFormat %Ss

    section vTaskDetect
    Read Sensors        :0, 0.02
    Check Threshold     :0.02, 0.02
    Update SharedData   :0.04, 0.01
    Signal Semaphores   :0.05, 0.01

    section vTaskTemperature
    Request Conversion  :0, 0.01
    Wait 800ms         :0.01, 0.8
    Read Temperature    :0.81, 0.01
    Median Filter       :0.82, 0.01
    Signal Display      :0.83, 0.01

    section vTaskDisplay
    Show Temperature    :0, 2
    Show Sound Level    :0, 2
    Show Temperature    :2, 5

    section vTaskLED
    Set RED (idle)      :0, 0.01
    Set GREEN (clap)    :0, 0.01
    Wait 1s             :0.01, 1
    Set RED (timeout)   :1.01, 0.01
    Set RED (idle)      :1.02, 5
```

## 24. State Machine - Temperature Task

```mermaid
stateDiagram-v2
    [*] --> Request

    Request --> Wait: requestTemperature()
    Wait --> Wait: waitCycles++
    Wait --> Read: waitCycles >= 8

    Read --> Filter: getTemperature()
    Filter --> Update: median calculation
    Update --> Signal: update SharedData
    Signal --> Request: semTempDisplay.give()
```

## 25. State Machine - Display Mode

```mermaid
stateDiagram-v2
    [*] --> Temperature

    Temperature --> Sound: clap detected<br/>(last_sound_time < 2s)
    Sound --> Temperature: 2s elapsed<br/>(last_sound_time >= 2s)

    note right of Temperature
        Display:
        Line 1: "Temp: XX.X C"
        Line 2: "Filt: XX.X C"
        RGB LED: RED
    end note

    note right of Sound
        Display:
        Line 1: "Sound --> XXXX"
        Line 2: "Clap!"
        RGB LED: GREEN
    end note
```

## 26. State Machine - RGB LED

```mermaid
stateDiagram-v2
    [*] --> Red

    Red --> Green: clap detected<br/>(semSoundLED)
    Green --> Red: 1s timeout<br/>(led_turn_off_time)

    note right of Red
        State: Idle
        Color: RED
        Pin 25: HIGH
        Pin 26: LOW
        Pin 27: LOW
    end note

    note right of Green
        State: Active
        Color: GREEN
        Pin 25: LOW
        Pin 26: HIGH
        Pin 27: LOW
        Duration: 1s
    end note
```

## 27. Median Filter Algorithm

```mermaid
flowchart TD
    Start((Start)) --> Add[Add new temp<br/>to buffer]
    Add --> Index[Update index<br/>(index + 1) % 5]
    Index --> Copy[Copy buffer to<br/>sorted array]
    Copy --> Sort1[Pass 1: Sort<br/>Compare & swap]
    Sort1 --> Sort2[Pass 2: Sort<br/>Compare & swap]
    Sort2 --> Sort3[Pass 3: Sort<br/>Compare & swap]
    Sort3 --> Sort4[Pass 4: Sort<br/>Compare & swap]
    Sort4 --> Median[Pick middle<br/>index 2]
    Median --> Output[Return filtered<br/>temperature]
    Output --> End((End))

    note1{Example:<br/>Raw: 26.5, 30.2, 26.3, 26.4, 26.6<br/>Sorted: 26.3, 26.4, 26.5, 26.6, 30.2<br/>Median: 26.5°C}
```

## 28. Task Synchronization Sequence Diagram

```mermaid
sequenceDiagram
    participant D as vTaskDetect
    participant T as vTaskTemperature
    participant SD as SharedData
    participant SemD as semSoundDisplay
    participant SemL as semSoundLED
    participant SemT as semTempDisplay
    participant Disp as vTaskDisplay
    participant LED as vTaskLED
    participant M as lcdMutex

    Note over D,M: System Operation

    par Parallel Execution
        D->>D: Read sound sensor
        D->>D: Check rising edge
        D->>D: Check 5s interval

        alt Clap detected
            D->>SD: Update sound data
            D->>SemD: give()
            D->>SemL: give()
        end

        Note over SemD,Disp: Display auto-switch logic

        opt < 2s since clap
            Disp->>SemD: take()
            Disp->>M: take()
            Disp->>LCD: Show sound
            Disp->>M: give()
        else >= 2s
            alt Temp available
                Disp->>SemT: take()
                Disp->>M: take()
                Disp->>LCD: Show temperature
                Disp->>M: give()
            end
        end

        Note over SemL,LED: LED timeout logic

        opt Semaphore signaled
            LED->>SemL: take()
            LED->>LED: Set GREEN
        end

        opt 1s elapsed
            LED->>LED: Check timeout
            LED->>LED: Set RED
        end
    and
        T->>T: Check state

        alt State: Request
            T->>T: requestTemperature()
            T->>T: Set state=Wait
        else State: Wait
            T->>T: waitCycles++
            alt waitCycles >= 8
                T->>T: Move to Read
            end
        else State: Read
            T->>T: getTemperature()
            T->>T: Median filter
            T->>SD: Update temp data
            T->>SemT: give()
            T->>T: Set state=Request
        end
    end

    Note over D,M: Next cycle
```