# Lab 3.2 - Dual Sensor System Documentation

## Overview

This directory contains comprehensive documentation and diagrams for the Dual Sensor System implementation.

## Files

### Documentation

- **`DUAL_SENSOR_SYSTEM_DOCUMENTATION.md`** - Complete technical documentation covering:
  - System architecture
  - Hardware components
  - Software modules
  - FreeRTOS tasks
  - Implementation details
  - Data flow
  - Configuration parameters
  - Troubleshooting guide

### Diagrams

- **`plant_uml/diagrams.md`** - All Mermaid diagrams (28 total)
- **`plant_uml/convert_all.py`** - Python script to convert Mermaid to PNG
- **`plant_uml/img/`** - Generated PNG images

### Diagram List (28 Total)

#### Architecture & Design
1. `01_architecture.png` - System architecture diagram
2. `02_component.png` - Component diagram
3. `03_class.png` - Class diagram
4. `10_detailed_architecture.png` - Detailed low-level architecture

#### Activity Diagrams
5. `04_activity_detect.png` - vTaskDetect activity
6. `05_activity_temperature.png` - vTaskTemperature activity
7. `06_activity_display.png` - vTaskDisplay activity
8. `07_activity_led.png` - vTaskLED activity

#### Data Flow & Layers
9. `08_dataflow.png` - Data flow diagram
10. `09_layers.png` - Software layers diagram

#### Sequence Diagrams
11. `11_sequence_sound_sensor.png` - Sound sensor sequence
12. `12_sequence_temperature_sensor.png` - Temperature sensor sequence
13. `13_sequence_lcd.png` - LCD sequence
14. `14_sequence_rgb_led.png` - RGB LED sequence

#### Detailed Data Flow
15. `15_dataflow_sound_input.png` - Sound input layer
16. `16_dataflow_temperature_input.png` - Temperature input layer
17. `17_dataflow_processing.png` - Processing layer
18. `18_dataflow_storage.png` - Storage layer
19. `19_dataflow_display.png` - Display output
20. `20_dataflow_rgb_led.png` - RGB LED output
21. `21_dataflow_synchronization.png` - Synchronization

#### Advanced Diagrams
22. `22_sequence_display_switch.png` - Display auto-switch sequence
23. `23_timeline.png` - Complete system timeline
24. `24_state_temperature.png` - Temperature task state machine
25. `25_state_display.png` - Display mode state machine
26. `26_state_rgb_led.png` - RGB LED state machine
27. `27_median_filter.png` - Median filter algorithm
28. `28_synchronization.png` - Task synchronization sequence

## How to Use

### Viewing Diagrams

1. **Markdown format**: Open `plant_uml/diagrams.md` in any Markdown viewer
2. **PNG images**: View generated images in `plant_uml/img/` folder
3. **Online viewer**: Copy Mermaid code to [Mermaid Live Editor](https://mermaid.live/)

### Regenerating PNG Images

If you need to regenerate the PNG images:

```bash
cd lib/lab3.2/plant_uml
python3 convert_all.py
```

**Requirements:**
- Python 3.x
- Internet connection (uses mermaid.ink API)
- `requests` library

## System Components

### Hardware
- **Sound Sensor**: Digital (GPIO12) + Analog (GPIO34)
- **Temperature Sensor**: DS18B20 (GPIO4)
- **RGB LED**: Red (GPIO25), Green (GPIO26), Blue (GPIO27)
- **LCD Display**: I2C (SDA: GPIO21, SCL: GPIO22)

### Software Tasks
- **vTaskDetect** (20ms, Priority 3): Sound detection
- **vTaskTemperature** (100ms, Priority 3): Temperature reading
- **vTaskDisplay** (100ms, Priority 2): LCD updates
- **vTaskLED** (Continuous, Priority 1): RGB LED control

### Key Features
- Rising edge detection for sound
- Median filter for temperature (5 samples)
- Non-blocking temperature reading (state machine)
- Auto-switch display (sound → temperature)
- RGB LED color changes (Red/Green)

## Configuration

### Sound Detection
- **Threshold**: 1500 (0-4095)
- **Minimum Interval**: 5 seconds
- **Display Duration**: 2 seconds

### Temperature
- **Resolution**: 12 bits (0.0625°C)
- **Conversion Time**: ~750ms
- **Filter**: Median (5 samples)

## Comparison with Lab 3.1

| Feature | Lab 3.1 | Lab 3.2 |
|---------|---------|---------|
| Sensors | Sound only | Sound + Temperature |
| Display | Count only | Sound level + Temperature |
| LED | Single LED | RGB LED |
| Tasks | 3 | 4 |
| Display Logic | Static | Dynamic auto-switch |
| Sound Detection | Threshold + hysteresis | Rising edge + interval |
| Temperature Reading | N/A | Non-blocking with filter |
| LED Behavior | Pulse ON/OFF | Color change (Red/Green) |

## Troubleshooting

See `DUAL_SENSOR_SYSTEM_DOCUMENTATION.md` for detailed troubleshooting guide.

## License

This documentation is part of the ES project (ESP32 Dual Sensor System).