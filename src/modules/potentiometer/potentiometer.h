#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>

/**
 * @class Potentiometer
 * @brief Analog input module for potentiometer reading
 * 
 * Features:
 * - Analog input reading (0-4095 for ESP32)
 * - Signal conditioning (debouncing, filtering)
 * - Percentage conversion (0-100%)
 * - Saturation to physical limits
 * - Median filtering for noise reduction
 * - Weighted averaging for fluctuation reduction
 * - Calibration support
 */
class Potentiometer {
public:
    /**
     * @brief Constructor
     * @param pin Analog input pin
     * @param minCalibration Minimum raw value for calibration
     * @param maxCalibration Maximum raw value for calibration
     */
    explicit Potentiometer(uint8_t pin, 
                          uint16_t minCalibration = 0, 
                          uint16_t maxCalibration = 4095);

    /**
     * @brief Initialize potentiometer
     */
    void begin();

    /**
     * @brief Read raw value (0-4095)
     * @return Raw ADC value
     */
    uint16_t readRaw();

    /**
     * @brief Read conditioned value with filtering
     * @return Conditioned value (0-4095)
     */
    uint16_t readConditioned();

    /**
     * @brief Read value as percentage (0-100%)
     * @return Value in percentage
     */
    uint8_t readPercentage();

    /**
     * @brief Scan and update internal state
     * Call this periodically in the control loop
     */
    void scan();

    /**
     * @brief Get current raw value
     * @return Last read raw value
     */
    uint16_t getRaw() const { return _rawValue; }

    /**
     * @brief Get current conditioned value
     * @return Last conditioned value
     */
    uint16_t getConditioned() const { return _conditionedValue; }

    /**
     * @brief Get current percentage
     * @return Last percentage value
     */
    uint8_t getPercentage() const { return _percentage; }

    /**
     * @brief Check if value changed significantly
     * @param threshold Change threshold (0-4095)
     * @return true if changed more than threshold
     */
    bool hasChanged(uint16_t threshold = 10);

    /**
     * @brief Set calibration range
     * @param minVal Minimum raw value
     * @param maxVal Maximum raw value
     */
    void setCalibration(uint16_t minVal, uint16_t maxVal);

    /**
     * @brief Enable/disable median filtering
     * @param enable true to enable, false to disable
     */
    void setMedianFilterEnabled(bool enable) { _medianFilterEnabled = enable; }

    /**
     * @brief Set median filter window size
     * @param size Window size (must be odd, max 9)
     */
    void setMedianFilterSize(uint8_t size);

private:
    uint8_t _pin;                 // Analog input pin
    uint16_t _rawValue;           // Current raw value
    uint16_t _conditionedValue;   // Conditioned value
    uint8_t _percentage;          // Percentage value (0-100)
    uint16_t _minCalibration;     // Minimum calibration value
    uint16_t _maxCalibration;     // Maximum calibration value

    // Filtering
    bool _medianFilterEnabled;
    uint8_t _medianFilterSize;
    uint16_t _medianFilterBuffer[9];
    uint8_t _medianFilterIndex;
    uint32_t _lastReadTime;
    uint32_t _readInterval;       // Read interval in ms

    /**
     * @brief Apply median filter
     * @return Median value
     */
    uint16_t applyMedianFilter(uint16_t value);

    /**
     * @brief Apply saturation to limits
     * @param value Input value
     * @return Saturated value
     */
    uint16_t saturate(uint16_t value);

    /**
     * @brief Convert raw value to percentage
     * @param value Raw value
     * @return Percentage (0-100)
     */
    uint8_t toPercentage(uint16_t value);
};

#endif // POTENTIOMETER_H