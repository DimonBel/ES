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

    // Read conditioned analog value: saturation + median + weighted average
    uint16_t readConditionedAnalog();
    
    // Check if sound is detected (digital)
    bool isSoundDetected();
    
    // Get current analog reading
    uint16_t getAnalogValue();

    // Get the latest raw (pre-filtered) analog reading
    uint16_t getRawAnalogValue() const;

    // Reset internal filter buffers
    void resetFilters();
    
    // Set threshold for analog detection
    void setThreshold(uint16_t threshold);
    uint16_t getThreshold() const;
    
    // Set hysteresis for anti-bouncing
    void setHysteresis(uint16_t hysteresis);
    uint16_t getHysteresis() const;

private:
    static constexpr uint16_t ADC_MIN = 0;
    static constexpr uint16_t ADC_MAX = 4095;
    static constexpr uint8_t MEDIAN_WINDOW_SIZE = 3;
    static constexpr uint8_t WEIGHTED_WINDOW_SIZE = 4;

    uint8_t _digitalPin;
    uint8_t _analogPin;
    uint16_t _threshold;
    uint16_t _hysteresis;
    uint16_t _currentValue;
    uint16_t _rawValue;
    bool _lastDigitalState;

    uint16_t _medianWindow[MEDIAN_WINDOW_SIZE];
    uint8_t _medianIndex;
    bool _medianFilled;

    uint16_t _weightedWindow[WEIGHTED_WINDOW_SIZE];
    uint8_t _weightedIndex;
    bool _weightedFilled;

    uint16_t saturate(uint16_t value) const;
    uint16_t computeMedian3(uint16_t a, uint16_t b, uint16_t c) const;
    uint16_t computeWeightedAverage() const;
};

#endif // SOUND_SENSOR_H