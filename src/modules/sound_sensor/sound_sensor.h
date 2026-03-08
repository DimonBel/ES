#ifndef SOUND_SENSOR_H
#define SOUND_SENSOR_H

#include <Arduino.h>

class SoundSensor
{
public:
    explicit SoundSensor(uint8_t digitalPin, uint8_t analogPin);
    void begin();
    
    // Read raw digital value (HIGH/LOW)
    bool readDigital();
    
    // Read raw analog value (0-4095 for ESP32)
    uint16_t readAnalog();
    
    // Check if sound is detected (digital)
    bool isSoundDetected();
    
    // Get current analog reading
    uint16_t getAnalogValue();
    
    // Set threshold for analog detection
    void setThreshold(uint16_t threshold);
    uint16_t getThreshold() const;
    
    // Set hysteresis for anti-bouncing
    void setHysteresis(uint16_t hysteresis);
    uint16_t getHysteresis() const;

private:
    uint8_t _digitalPin;
    uint8_t _analogPin;
    uint16_t _threshold;
    uint16_t _hysteresis;
    uint16_t _currentValue;
    bool _lastDigitalState;
};

#endif // SOUND_SENSOR_H