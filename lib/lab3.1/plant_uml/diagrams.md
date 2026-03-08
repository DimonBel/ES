# Sound Detection System - UML Diagrams

## 1. Architecture Diagram

```mermaid
graph TB
    subgraph Hardware["Hardware Components"]
        Sensor[Sound Sensor]
        LED[LED]
        LCD[LCD]
    end

    subgraph Tasks["FreeRTOS Tasks"]
        Detect[vTaskDetect<br/>20ms, P3]
        Display[vTaskDisplay<br/>500ms, P2]
        LEDTask[vTaskLED<br/>P1]
    end

    subgraph Sync["Synchronization"]
        Sem[Semaphores]
        Mutex[Mutex]
    end

    Sensor -->|reads| Detect
    Detect -->|signals| Sem
    Sem -->|triggers| Display
    Sem -->|triggers| LEDTask
    Display -->|locks| Mutex
    Display -->|updates| LCD
    LEDTask -->|controls| LED

    note1{Thresh: 2000<br/>Debounce: 50ms}
```

## 2. Component Diagram

```mermaid
graph TB
    subgraph HW["Hardware"]
        SoundSensor[SoundSensor]
        Led[LED]
        LCDComp[LCD]
    end

    subgraph App["FreeRTOS App"]
        DetectComp[Detect]
        DisplayComp[Display]
        LEDComp[LED]
        SharedData[SharedData]
    end

    subgraph SyncComp["Sync"]
        SyncMutex[Mutex]
        Semaphore[Semaphore]
    end

    DetectComp --> SoundSensor
    DetectComp --> SharedData
    DisplayComp --> LCDComp
    DisplayComp --> SyncMutex
    LEDComp --> Led
    LEDComp --> Semaphore

    note1{SharedData:<br/>sound_count<br/>led_state<br/>timestamp}
```

## 3. Class Diagram

```mermaid
classDiagram
    class SoundSensor {
        +readDigital() bool
        +readAnalog() uint16
        +isSoundDetected() bool
    }

    class Led {
        +on() void
        +off() void
        +toggle() void
    }

    class LcdI2c {
        +print(text) void
        +clear() void
        +setCursor(col, row) void
    }

    class SharedData {
        +sound_count uint32
        +led_state bool
        +timestamp uint32
    }

    class vTaskDetect {
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
    vTaskDisplay --> LcdI2c : uses
    vTaskLED --> Led : uses
```

## 4. Detect Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> Wait[Wait 20ms]
    Wait --> Read[Read sensor]
    Read --> Check{sound<br/>detected?}

    Check -->|yes| Increment[Increment count]
    Increment --> SignalLED[Signal LED]
    SignalLED --> SignalDisplay[Signal Display]
    SignalDisplay --> Loop

    Check -->|no| Loop((Loop back))
    Loop --> Wait
```

## 5. Display Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> Wait[Wait 500ms]
    Wait --> Format[Format display]
    Format --> Lock[Lock mutex]
    Lock --> Update[Update LCD]
    Update --> Release[Release mutex]
    Release --> Loop((Loop back))
    Loop --> Wait
```

## 6. LED Activity Diagram

```mermaid
flowchart TD
    Start((Start)) --> TimeoutCheck{timeout<br/>reached?}
    TimeoutCheck -->|yes| TurnOFF[Turn LED OFF]
    TurnOFF --> SemaphoreCheck

    TimeoutCheck -->|no| SemaphoreCheck

    SemaphoreCheck{semaphore<br/>signaled?}
    SemaphoreCheck -->|yes| TurnON[Turn LED ON]
    TurnON --> Delay[Delay 50ms]

    SemaphoreCheck -->|no| Delay
    Delay --> Loop((Loop back))
    Loop --> TimeoutCheck
```

## 7. Data Flow Diagram

```mermaid
graph LR
    Sensor[Sensor] -->|analog/digital| Detect[Detect]
    Detect -->|count/state| SharedData[SharedData]
    SharedData -->|count| Display[Display]
    SharedData -->|led_state| LED[LED]
    Display -->|Count: X| LCD[LCD]
    LED -->|ON 1s/OFF| LEDHW[LED HW]

    note1{Thresh: 2000}
```

## 8. Software Layers Diagram

```mermaid
graph TB
    subgraph Application["Application Layer"]
        AppTasks[Detect, Display, LED]
    end

    subgraph HAL["Hardware Abstraction Layer"]
        HWSensor[Sensor]
        HWLED[LED]
        HWLCD[LCD]
    end

    subgraph FreeRTOS["FreeRTOS Kernel"]
        RTOSTasks[Task]
        RTOSMutex[Mutex]
        RTOSSem[Semaphore]
    end

    Application -->|uses| HAL
    Application -->|creates| FreeRTOS

    note{3 Tasks:<br/>20ms, 500ms,<br/>Continuous}
```

## 9. Detailed Low-Level Architecture

```mermaid
graph TB
    ESP32_Hardware[ESP32 Hardware<br/>ADC0: Pin 32<br/>GPIO12: Digital<br/>GPIO14: LED<br/>I2C: SDA21/SCL22]
    
    HAL[HAL Layer<br/>SoundSensor<br/>_digitalPin: 12<br/>_analogPin: 32<br/>_threshold: 2000<br/>_hysteresis: 100<br/><br/>Led<br/>_pin: 14<br/>_state: bool<br/><br/>LcdI2c<br/>_address: 0x27<br/>_cols: 16<br/>_rows: 2]
    
    Tasks[FreeRTOS Tasks<br/>vTaskDetect: 20ms P3<br/>Stack: 4096<br/><br/>vTaskDisplay: 500ms P2<br/>Stack: 4096<br/><br/>vTaskLED: Continuous P1<br/>Stack: 4096]
    
    Sync[Sync Primitives<br/>lcdMutex: Mutex<br/>Timeout: 100ms<br/>Protects: LCD I2C<br/><br/>semSoundDisplay: Binary<br/>Trigger: Display<br/><br/>semSoundLED: Binary<br/>Trigger: LED]
    
    Data[SharedData<br/>uint16_t analog_value<br/>bool sound_detected<br/>uint32_t sound_count<br/>bool led_state<br/>uint32_t led_turn_off_time<br/>uint32_t last_sound_time<br/>bool threshold_exceeded]

    Config[Configuration<br/>THRESHOLD: 2000<br/>HYSTERESIS: 100<br/>DEBOUNCE: 50ms<br/>LED_TIMEOUT: 1000ms]

    ESP32_Hardware -->|GPIO/ADC| HAL
    HAL -->|methods| Tasks
    Tasks -->|uses| Sync
    Tasks -->|updates| Data
    Sync -->|protects| Data
    Config -->|used by| Tasks
```

## 10. Sound Sensor Sequence Diagram

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
    SS->>HW: analogRead(PIN 32)
    HW-->>SS: 0-4095 (uint16)
    SS-->>T: 0-4095 (uint16)

    T->>SS: isSoundDetected()
    SS->>SS: Check digital edge<br/>return HIGH && !lastState
    SS-->>T: true/false (bool)

    alt Sound Detected (true returned)
        T->>SD: sound_count++
        SD-->>T: new count (uint32)
        
        T->>SD: led_state = true
        SD-->>T: confirmed (bool)
        
        T->>SD: led_turn_off_time = now + 1000ms
        SD-->>T: timestamp (uint32)
        
        T->>SD: last_sound_time = now
        SD-->>T: timestamp (uint32)
        
        T->>SD: threshold_exceeded = true
        SD-->>T: confirmed (bool)
        
        T->>SD: analog_value = current_value
        SD-->>T: stored (bool)
        
        T->>T: give(semSoundDisplay)
        T-->>T: success (bool)
        
        T->>T: give(semSoundLED)
        T-->>T: success (bool)
    else No Sound (false returned)
        T->>SD: threshold_exceeded = false
        SD-->>T: confirmed (bool)
        
        T->>SD: led_state = false
        SD-->>T: confirmed (bool)
        
        T->>SD: analog_value = current_value
        SD-->>T: stored (bool)
    end

    Note over T,HW: Wait until next 20ms period
```

## 11. LCD Sequence Diagram

```mermaid
sequenceDiagram
    participant D as vTaskDisplay
    participant SD as SharedData
    participant M as lcdMutex
    participant LCD as LcdI2c
    participant I2C as I2C Bus

    Note over D,I2C: Every 500ms cycle

    D->>D: Wait 500ms
    D-->>D: elapsed (bool)
    
    D->>D: Format display string<br/>snprintf("Count: %lu", count)
    D-->>D: string length (int)

    D->>M: take(100ms)
    activate M
    M-->>D: true (success)
    deactivate M

    D->>SD: Read sound_count
    SD-->>D: count value (uint32)

    D->>LCD: clear()
    activate LCD
    LCD->>I2C: I2C write 0x01 (clear command)
    I2C-->>LCD: ACK (0x00)
    LCD-->>D: cleared (void)
    deactivate LCD

    D->>LCD: setCursor(0, 0)
    activate LCD
    LCD->>I2C: I2C write 0x80 (DDRAM address)
    I2C-->>LCD: ACK (0x00)
    LCD->>I2C: I2C write 0x00 (row 0, col 0)
    I2C-->>LCD: ACK (0x00)
    LCD-->>D: positioned (void)
    deactivate LCD

    D->>LCD: print("Count:")
    activate LCD
    loop Each character "Count:"
        LCD->>I2C: I2C write character
        I2C-->>LCD: ACK (0x00)
    end
    LCD-->>D: written (int)
    deactivate LCD

    D->>LCD: setCursor(0, 1)
    activate LCD
    LCD->>I2C: I2C write 0xC0 (DDRAM address)
    I2C-->>LCD: ACK (0x00)
    LCD-->>D: positioned (void)
    deactivate LCD

    D->>LCD: print(count_str)
    activate LCD
    loop Each character of count
        LCD->>I2C: I2C write character
        I2C-->>LCD: ACK (0x00)
    end
    LCD-->>D: written (int)
    deactivate LCD

    D->>M: give()
    activate M
    M-->>D: released (bool)
    deactivate M

    Note over D,I2C: Wait until next 500ms period
```

## 12. LED Sequence Diagram

```mermaid
sequenceDiagram
    participant L as vTaskLED
    participant SD as SharedData
    participant S as semSoundLED
    participant LED as Led
    participant HW as GPIO 14

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
            L->>LED: on()
            activate LED
            LED->>HW: digitalWrite(PIN 14, HIGH)
            HW-->>LED: success (void)
            LED->>LED: _state = true
            LED-->>L: void
            deactivate LED
        else LED should be OFF (false)
            L->>LED: off()
            activate LED
            LED->>HW: digitalWrite(PIN 14, LOW)
            HW-->>LED: success (void)
            LED->>LED: _state = false
            LED-->>L: void
            deactivate LED
        end
    end

    alt Timeout check (every 50ms)
        L->>SD: Read led_turn_off_time
        SD-->>L: timestamp (uint32)

        L->>L: currentTime = xTaskGetTickCount()
        L-->>L: current ticks (TickType_t)

        alt currentTime >= led_turn_off_time
            L->>SD: led_state = false
            SD-->>L: confirmed (bool)
            
            L->>LED: off()
            activate LED
            LED->>HW: digitalWrite(PIN 14, LOW)
            HW-->>LED: success (void)
            LED->>LED: _state = false
            LED-->>L: void
            deactivate LED
        else Keep LED ON
            L->>SD: Read led_state
            SD-->>L: true (still on)
        end
    end

    Note over L,HW: Loop continues every 50ms
```

## 13. Data Flow - Input Layer

```mermaid
graph LR
    subgraph Hardware_Input["Hardware Input Layer"]
        ADC[ADC0<br/>Pin: 32<br/>Function: Read analog<br/>Returns: 0-4095<br/>ESP32 ADC1_CH4]
        
        DIG[GPIO 12<br/>Function: Read digital<br/>Returns: HIGH/LOW<br/>Sound Sensor D0]
        
        SOUND[Sound Sensor HW<br/>Function: Detect sound<br/>Output: Digital/Analog<br/>Threshold: Adjustable]
    end

    subgraph HAL_Input["HAL Input Layer"]
        Sensor[SoundSensor<br/>readDigital(): digitalRead 12<br/>readAnalog(): analogRead 32<br/>isSoundDetected(): edge detection<br/>_lastDigitalState: bool<br/>_currentValue: uint16]
    end

    subgraph Task_Input["Task Input Layer"]
        Detect[vTaskDetect<br/>Function: Read sensors<br/>Period: 20ms<br/>Priority: 3<br/>Calls: Sensor methods]
    end

    SOUND -->|analog signal| ADC
    SOUND -->|digital signal| DIG
    
    ADC -->|value 0-4095| Sensor
    DIG -->|HIGH/LOW| Sensor
    
    Sensor -->|bool HIGH/LOW| Detect
    Sensor -->|uint16 0-4095| Detect
    Sensor -->|bool soundDetected| Detect
```

## 14. Data Flow - Processing Layer

```mermaid
graph TB
    subgraph Detection_Task["vTaskDetect Processing"]
        Read[Read Sensors<br/>Function: Call SoundSensor methods<br/>Calls: readDigital, readAnalog<br/>Returns: bool, uint16]
        
        Check[Edge Detection<br/>Function: Check rising edge<br/>Logic: current && !last<br/>Returns: bool true/false]
        
        Compare[Threshold Compare<br/>Function: Compare value to threshold<br/>Logic: value > 2000<br/>Hysteresis: >1900 if LED ON]
        
        Update[Update SharedData<br/>Function: sound_count++<br/>led_state = true<br/>led_turn_off_time = now+1000ms]
        
        Signal[Signal Tasks<br/>Function: give semaphores<br/>semSoundDisplay.give<br/>semSoundLED.give]
    end

    subgraph Timing["Timing Control"]
        Timer[20ms Timer<br/>Function: vTaskDelayUntil<br/>Returns: after 20ms<br/>Ensures: 50Hz rate]
        
        Debounce[Debounce 50ms<br/>Function: Check time elapsed<br/>Logic: now - last >= 50ms<br/>Purpose: Filter noise]
    end

    subgraph Constants["Configuration"]
        Config[Constants<br/>SOUND_THRESHOLD: 2000<br/>SOUND_HYSTERESIS: 100<br/>SOUND_DEBOUNCE: 50ms<br/>LED_TIMEOUT: 1000ms]
    end

    Read --> Check
    Check --> Compare
    Compare --> Update
    Update --> Signal
    
    Timer --> Read
    Debounce --> Check
    
    Config --> Compare
    Config --> Debounce
```

## 15. Data Flow - Storage Layer

```mermaid
graph TB
    subgraph Shared_Data["SharedData Structure"]
        Analog[analog_value<br/>Type: uint16<br/>Purpose: Store current ADC<br/>Writer: vTaskDetect<br/>Range: 0-4095]
        
        Digital[sound_detected<br/>Type: bool<br/>Purpose: Digital edge flag<br/>Writer: vTaskDetect<br/>Values: true/false]
        
        Threshold[threshold_exceeded<br/>Type: bool<br/>Purpose: State flag<br/>Writer: vTaskDetect<br/>Values: true/false]
        
        Count[sound_count<br/>Type: uint32<br/>Purpose: Event counter<br/>Writer: vTaskDetect<br/>Reader: vTaskDisplay]
        
        LED_State[led_state<br/>Type: bool<br/>Purpose: LED ON/OFF<br/>Writer: vTaskDetect<br/>Reader: vTaskLED]
        
        LED_Time[led_turn_off_time<br/>Type: uint32<br/>Purpose: 1s timeout<br/>Writer: vTaskDetect<br/>Reader: vTaskLED]
        
        Last_Time[last_sound_time<br/>Type: uint32<br/>Purpose: Last event<br/>Writer: vTaskDetect<br/>Unit: ticks]
        
        Task_State[task_state<br/>Type: uint8<br/>Purpose: Debug state<br/>Writer: vTaskDetect<br/>Values: 0/1]
    end

    subgraph Memory["Memory Access"]
        Write[Write: vTaskDetect<br/>Period: 20ms<br/>Priority: 3<br/>Writes: All fields]
        
        ReadDisplay[Read: vTaskDisplay<br/>Period: 500ms<br/>Priority: 2<br/>Reads: sound_count]
        
        ReadLED[Read: vTaskLED<br/>Continuous<br/>Priority: 1<br/>Reads: led_state<br/>Reads: led_turn_off_time]
    end

    Write -->|store| Analog
    Write -->|store| Digital
    Write -->|store| Threshold
    Write -->|increment| Count
    Write -->|set| LED_State
    Write -->|set| LED_Time
    Write -->|set| Last_Time
    Write -->|set| Task_State
    
    ReadDisplay -->|read| Count
    
    ReadLED -->|read| LED_State
    ReadLED -->|read| LED_Time
```

## 16. Data Flow - Display Output

```mermaid
graph LR
    subgraph Display_Task["vTaskDisplay Output"]
        Wait[Wait 500ms<br/>Function: vTaskDelayUntil<br/>Period: 500ms<br/>Priority: 2]
        
        Read[Read SharedData<br/>Function: Read sound_count<br/>Variable: sharedData.sound_count<br/>Type: uint32]
        
        Format[Format String<br/>Function: snprintf<br/>Format: Count: %lu<br/>Buffer: 16 chars]
        
        Lock[Lock Mutex<br/>Function: lcdMutex.take<br/>Timeout: 100ms<br/>Purpose: Protect I2C]
    end

    subgraph LCD_Hardware["LCD Hardware"]
        I2C[I2C Bus<br/>SDA: Pin 21<br/>SCL: Pin 22<br/>Address: 0x27<br/>Speed: 100kHz]
        
        LCD[LCD Controller<br/>Type: PCF8574<br/>Size: 16x2 chars<br/>Function: Display text]
        
        Display[Display Buffer<br/>Line 1: Count:<br/>Line 2: 12345<br/>Size: 32 bytes]
    end

    subgraph LCD_Ops["LCD Operations"]
        Clear[Clear Screen<br/>Command: 0x01<br/>Function: lcd.clear<br/>Delay: 2ms]
        
        Cursor[Set Cursor<br/>Row 0: 0x80<br/>Row 1: 0xC0<br/>Function: lcd.setCursor]
        
        Write[Write Text<br/>Function: lcd.print<br/>Chars: ASCII<br/>Returns: void]
    end

    Wait --> Read
    Read --> Format
    Format --> Lock
    
    Lock --> I2C
    I2C --> LCD
    LCD --> Display
    
    Display --> Clear
    Display --> Cursor
    Display --> Write
    
    Unlock[Unlock Mutex<br/>Function: lcdMutex.give<br/>Release I2C<br/>Next task]
    
    Write --> Unlock
    Unlock --> I2C
```

## 17. Data Flow - LED Output

```mermaid
graph LR
    subgraph LED_Task["vTaskLED Output"]
        Trigger[Trigger<br/>semSoundLED<br/>Binary Semaphore<br/>Signal from Detect]
        
        Loop[Loop<br/>Period: 50ms<br/>Priority: 1<br/>Continuous]
        
        CheckState[Check State<br/>Read led_state<br/>Read timestamp<br/>Compare current]
        
        Control[Control GPIO<br/>Pin: 14<br/>Type: Output<br/>Mode: Push-pull]
    end

    subgraph LED_Hardware["LED Hardware"]
        GPIO[GPIO 14<br/>ESP32 GPIO<br/>Direction: OUT<br/>Drive strength: High]
        
        LED_HW[LED HW<br/>Anode: GPIO 14<br/>Cathode: GND<br/>Resistor: 220Ω<br/>Color: Red]
        
        State[LED State<br/>OFF: LOW (0V)<br/>ON: HIGH (3.3V)<br/>Duration: 1000ms]
    end

    subgraph LED_Logic["LED Logic"]
        TurnON[Turn ON<br/>digitalWrite 14 HIGH<br/>Set led_state=true<br/>Start timeout]
        
        TurnOFF[Turn OFF<br/>digitalWrite 14 LOW<br/>Set led_state=false<br/>Clear timeout]
        
        Timeout[Timeout Check<br/>led_turn_off_time<br/>Current time<br/>Compare: >=]
    end

    Trigger --> Loop
    Loop --> CheckState
    CheckState --> Control
    
    Control --> GPIO
    GPIO --> LED_HW
    LED_HW --> State
    
    State -->|signal received| TurnON
    TurnON --> GPIO
    
    State -->|timeout reached| Timeout
    Timeout --> TurnOFF
    TurnOFF --> GPIO
    
    Time[Timing<br/>ON duration: 1000ms<br/>Check interval: 50ms<br/>Priority: Low]
    
    Loop --> Time
```

## 18. Data Flow - Synchronization

```mermaid
graph TB
    subgraph Semaphores["Binary Semaphores"]
        Sem1[semSoundDisplay<br/>Type: Binary Semaphore<br/>Handle: SemaphoreHandle_t<br/>Creator: Detect<br/>Waiter: Display<br/>Trigger: Sound event]
        
        Sem2[semSoundLED<br/>Type: Binary Semaphore<br/>Handle: SemaphoreHandle_t<br/>Creator: Detect<br/>Waiter: LED<br/>Trigger: Sound event]
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
        
        Display[Display Task<br/>Priority: 2<br/>Period: 500ms<br/>Signal waiter<br/>Max wait: 500ms]
        
        LED[LED Task<br/>Priority: 1<br/>Continuous<br/>Signal waiter<br/>Max wait: 0ms]
    end

    Detect -->|sound detected| Sem1
    Detect -->|sound detected| Sem2
    
    Sem1 -->|trigger| Display
    Sem2 -->|trigger| LED
    
    Display -->|take| Sem1
    LED -->|take| Sem2
    
    Sem1 -->|give| Detect
    Sem2 -->|give| Detect
    
    Display -->|lock| Mutex1
    Mutex1 -->|unlock| Display
    
    Give --> Sem1
    Take --> Sem1
    Give --> Sem2
    Take --> Sem2
    
    Sync[Sync Pattern<br/>Detect: Signal<br/>Display: Wait/Update<br/>LED: Wait/Control<br/>LCD: Lock/Write]
    
    Detect --> Sync
    Display --> Sync
    LED --> Sync
```

## 19. Task Synchronization Sequence Diagram

```mermaid
sequenceDiagram
    participant D as vTaskDetect
    participant SD as SharedData
    participant SemD as semSoundDisplay
    participant SemL as semSoundLED
    participant Disp as vTaskDisplay
    participant LED as vTaskLED
    participant M as lcdMutex

    Note over D,M: Sound Detection Event (20ms cycle)

    D->>D: Read sensor
    D-->>D: values (uint16, bool)
    
    D->>D: Check threshold (>2000)
    D-->>D: exceeded (bool)
    
    D->>D: Check hysteresis (>1900 if ON)
    D-->>D: should trigger (bool)

    alt Sound detected
        D->>SD: Update sound_count++
        SD-->>D: new count (uint32)
        
        D->>SD: led_state = true
        SD-->>D: confirmed (bool)
        
        D->>SD: led_turn_off_time = now + 1000ms
        SD-->>D: timestamp (uint32)
        
        D->>SD: analog_value = current_value
        SD-->>D: stored (bool)

        par Signal both tasks
            D->>SemD: give()
            SemD-->>D: success (bool)
            D->>SemL: give()
            SemL-->>D: success (bool)
        end

        Note over SemD,Disp: Display task blocked on semaphore

        opt Display task wakes (up to 500ms delay)
            Disp->>SemD: take()
            SemD-->>Disp: true (unblocked)
            
            Disp->>M: take(100ms)
            M-->>Disp: true (locked)
            
            Disp->>SD: Read sound_count
            SD-->>Disp: count (uint32)
            
            Disp->>Disp: Format string
            Disp-->>Disp: formatted (char*)
            
            Disp->>Disp: Update LCD
            Disp-->>Disp: success (void)
            
            Disp->>M: give()
            M-->>Disp: released (bool)
        end

        opt LED task wakes (immediate)
            LED->>SemL: take()
            SemL-->>LED: true (unblocked)
            
            LED->>SD: Read led_state
            SD-->>LED: true (on)
            
            LED->>LED: Turn LED ON
            LED-->>LED: done (void)
        end

        Note over LED,SD: After 1000ms (LED timeout)
        LED->>SD: Check led_turn_off_time
        SD-->>LED: timestamp (uint32)
        
        LED->>LED: Timeout reached
        LED-->>LED: true
        
        LED->>SD: led_state = false
        SD-->>LED: confirmed (bool)
        
        LED->>LED: Turn LED OFF
        LED-->>LED: done (void)
    end

    Note over D,M: Next 20ms cycle
```