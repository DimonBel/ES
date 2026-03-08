#include "sound_sensor.h"

SoundSensor::SoundSensor(uint8_t digitalPin, uint8_t analogPin)
    : _digitalPin(digitalPin), 
      _analogPin(analogPin),
      _threshold(2000),       // Default threshold (0-4095)
      _hysteresis(100),       // Default hysteresis
      _currentValue(0),
      _lastDigitalState(false)
{
}

void SoundSensor::begin()
{
    pinMode(_digitalPin, INPUT);
    // Analog pin doesn't need pinMode on ESP32
    _currentValue = readAnalog();
    _lastDigitalState = readDigital();
}

bool SoundSensor::readDigital()
{
    return digitalRead(_digitalPin) == HIGH;
}

uint16_t SoundSensor::readAnalog()
{
    _currentValue = analogRead(_analogPin);
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