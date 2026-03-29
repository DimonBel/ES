#include "potentiometer.h"
#include <stdio.h>
#include <algorithm>

Potentiometer::Potentiometer(uint8_t pin, uint16_t minCalibration, uint16_t maxCalibration)
    : _pin(pin)
    , _rawValue(0)
    , _conditionedValue(0)
    , _percentage(0)
    , _minCalibration(minCalibration)
    , _maxCalibration(maxCalibration)
    , _medianFilterEnabled(true)
    , _medianFilterSize(5)
    , _medianFilterIndex(0)
    , _lastReadTime(0)
    , _readInterval(10) // Read every 10ms
{
    // Initialize median filter buffer
    for (uint8_t i = 0; i < 9; i++) {
        _medianFilterBuffer[i] = 0;
    }
}

void Potentiometer::begin() {
    // Configure analog pin
    pinMode(_pin, INPUT);

    // Initial reading
    _rawValue = analogRead(_pin);
    _conditionedValue = _rawValue;
    _percentage = toPercentage(_conditionedValue);

    // Initialize median filter buffer with initial value
    for (uint8_t i = 0; i < _medianFilterSize; i++) {
        _medianFilterBuffer[i] = _rawValue;
    }

    _lastReadTime = millis();

    printf("[POTENTIOMETER] Initialized on pin %d\n", _pin);
    printf("[POTENTIOMETER] Calibration range: %d - %d\n", 
           _minCalibration, _maxCalibration);
    printf("[POTENTIOMETER] Initial value: %d (%d%%)\n", 
           _rawValue, _percentage);
}

uint16_t Potentiometer::readRaw() {
    _rawValue = analogRead(_pin);
    return _rawValue;
}

uint16_t Potentiometer::readConditioned() {
    readRaw();

    // Apply median filter if enabled
    if (_medianFilterEnabled) {
        _conditionedValue = applyMedianFilter(_rawValue);
    } else {
        _conditionedValue = _rawValue;
    }

    // Apply saturation
    _conditionedValue = saturate(_conditionedValue);

    // Calculate percentage
    _percentage = toPercentage(_conditionedValue);

    return _conditionedValue;
}

uint8_t Potentiometer::readPercentage() {
    readConditioned();
    return _percentage;
}

void Potentiometer::scan() {
    uint32_t currentTime = millis();

    // Read at configured interval
    if (currentTime - _lastReadTime >= _readInterval) {
        _lastReadTime = currentTime;
        readConditioned();
    }
}

bool Potentiometer::hasChanged(uint16_t threshold) {
    scan();
    return abs((int32_t)_conditionedValue - (int32_t)_rawValue) >= threshold;
}

void Potentiometer::setCalibration(uint16_t minVal, uint16_t maxVal) {
    if (minVal >= maxVal) {
        printf("[POTENTIOMETER] ERROR: Invalid calibration range\n");
        return;
    }

    _minCalibration = minVal;
    _maxCalibration = maxVal;

    printf("[POTENTIOMETER] Calibration updated: %d - %d\n", 
           _minCalibration, _maxCalibration);
}

void Potentiometer::setMedianFilterSize(uint8_t size) {
    // Ensure size is odd and within valid range
    if (size < 3) size = 3;
    if (size > 9) size = 9;
    if (size % 2 == 0) size++; // Make it odd

    _medianFilterSize = size;
    _medianFilterIndex = 0;

    // Reset buffer
    for (uint8_t i = 0; i < 9; i++) {
        _medianFilterBuffer[i] = _rawValue;
    }

    printf("[POTENTIOMETER] Median filter size set to %d\n", size);
}

uint16_t Potentiometer::applyMedianFilter(uint16_t value) {
    // Store new value in buffer
    _medianFilterBuffer[_medianFilterIndex] = value;
    _medianFilterIndex = (_medianFilterIndex + 1) % _medianFilterSize;

    // Create temporary buffer for sorting
    uint16_t tempBuffer[9];
    for (uint8_t i = 0; i < _medianFilterSize; i++) {
        tempBuffer[i] = _medianFilterBuffer[i];
    }

    // Sort the buffer
    std::sort(tempBuffer, tempBuffer + _medianFilterSize);

    // Return median value
    return tempBuffer[_medianFilterSize / 2];
}

uint16_t Potentiometer::saturate(uint16_t value) {
    if (value < _minCalibration) {
        return _minCalibration;
    }
    if (value > _maxCalibration) {
        return _maxCalibration;
    }
    return value;
}

uint8_t Potentiometer::toPercentage(uint16_t value) {
    if (_maxCalibration == _minCalibration) {
        return 0;
    }

    uint32_t range = _maxCalibration - _minCalibration;
    uint32_t adjustedValue = value - _minCalibration;

    return (adjustedValue * 100) / range;
}