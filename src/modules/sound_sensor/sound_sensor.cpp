#include "sound_sensor.h"

SoundSensor::SoundSensor(uint8_t digitalPin, uint8_t analogPin)
    : _digitalPin(digitalPin), 
      _analogPin(analogPin),
      _threshold(2000),       // Default threshold (0-4095)
      _hysteresis(100),       // Default hysteresis
      _currentValue(0),
            _rawValue(0),
            _lastDigitalState(false),
            _medianWindow{0, 0, 0},
            _medianIndex(0),
            _medianFilled(false),
            _weightedWindow{0, 0, 0, 0},
            _weightedIndex(0),
            _weightedFilled(false)
{
}

void SoundSensor::begin()
{
    pinMode(_digitalPin, INPUT);
    // Analog pin doesn't need pinMode on ESP32
        readAnalog();
        resetFilters();
    _lastDigitalState = readDigital();
}

bool SoundSensor::readDigital()
{
    return digitalRead(_digitalPin) == HIGH;
}

uint16_t SoundSensor::readAnalog()
{
    _rawValue = analogRead(_analogPin);
    _currentValue = _rawValue;
    return _rawValue;
}

uint16_t SoundSensor::readConditionedAnalog()
{
    const uint16_t raw = readAnalog();
    const uint16_t saturated = saturate(raw);

    _medianWindow[_medianIndex] = saturated;
    _medianIndex = (_medianIndex + 1) % MEDIAN_WINDOW_SIZE;
    if (_medianIndex == 0) {
        _medianFilled = true;
    }

    uint16_t medianValue = saturated;
    if (_medianFilled) {
        medianValue = computeMedian3(_medianWindow[0], _medianWindow[1], _medianWindow[2]);
    }

    _weightedWindow[_weightedIndex] = medianValue;
    _weightedIndex = (_weightedIndex + 1) % WEIGHTED_WINDOW_SIZE;
    if (_weightedIndex == 0) {
        _weightedFilled = true;
    }

    if (_weightedFilled) {
        _currentValue = computeWeightedAverage();
    } else {
        _currentValue = medianValue;
    }

    return _currentValue;
}

bool SoundSensor::isSoundDetected()
{
    bool currentDigitalState = readDigital();
    
    // Simple edge detection - return true on rising edge
    if (currentDigitalState && !_lastDigitalState) {
        _lastDigitalState = currentDigitalState;
        return true;
    }
    
    _lastDigitalState = currentDigitalState;
    return false;
}

uint16_t SoundSensor::getAnalogValue()
{
    return _currentValue;
}

uint16_t SoundSensor::getRawAnalogValue() const
{
    return _rawValue;
}

void SoundSensor::resetFilters()
{
    for (uint8_t i = 0; i < MEDIAN_WINDOW_SIZE; i++) {
        _medianWindow[i] = _currentValue;
    }
    _medianIndex = 0;
    _medianFilled = true;

    for (uint8_t i = 0; i < WEIGHTED_WINDOW_SIZE; i++) {
        _weightedWindow[i] = _currentValue;
    }
    _weightedIndex = 0;
    _weightedFilled = true;
}

void SoundSensor::setThreshold(uint16_t threshold)
{
    if (threshold > 4095) threshold = 4095;
    _threshold = threshold;
}

uint16_t SoundSensor::getThreshold() const
{
    return _threshold;
}

void SoundSensor::setHysteresis(uint16_t hysteresis)
{
    _hysteresis = hysteresis;
}

uint16_t SoundSensor::getHysteresis() const
{
    return _hysteresis;
}

uint16_t SoundSensor::saturate(uint16_t value) const
{
    if (value < ADC_MIN) return ADC_MIN;
    if (value > ADC_MAX) return ADC_MAX;
    return value;
}

uint16_t SoundSensor::computeMedian3(uint16_t a, uint16_t b, uint16_t c) const
{
    if (a > b) {
        uint16_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        uint16_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        uint16_t t = a;
        a = b;
        b = t;
    }
    return b;
}

uint16_t SoundSensor::computeWeightedAverage() const
{
    // Higher weight for recent samples to keep fast response.
    static const uint8_t weights[WEIGHTED_WINDOW_SIZE] = {1, 2, 3, 4};

    uint32_t weightedSum = 0;
    uint32_t totalWeight = 0;

    const uint8_t newestIndex = (_weightedIndex + WEIGHTED_WINDOW_SIZE - 1) % WEIGHTED_WINDOW_SIZE;
    for (uint8_t age = 0; age < WEIGHTED_WINDOW_SIZE; age++) {
        const uint8_t idx = (newestIndex + WEIGHTED_WINDOW_SIZE - age) % WEIGHTED_WINDOW_SIZE;
        const uint8_t weight = weights[WEIGHTED_WINDOW_SIZE - 1 - age];
        weightedSum += static_cast<uint32_t>(_weightedWindow[idx]) * weight;
        totalWeight += weight;
    }

    return static_cast<uint16_t>(weightedSum / totalWeight);
}