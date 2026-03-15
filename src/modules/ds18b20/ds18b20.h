#ifndef DS18B20_H
#define DS18B20_H

#include <Arduino.h>
#include <OneWire.h>

class DS18B20 {
public:
    explicit DS18B20(uint8_t pin);
    ~DS18B20();

    void begin();
    
    // Temperature reading methods
    float readTemperature();
    bool requestTemperature();
    bool isConversionComplete();
    float getTemperature();
    
    // Configuration
    void setResolution(uint8_t resolution);  // 9, 10, 11, or 12 bits
    bool isPresent();
    
private:
    uint8_t _pin;
    OneWire* _oneWire;
    uint8_t _address[8];
    bool _deviceFound;
    uint8_t _resolution;
    
    // Timing constants for conversion
    unsigned long _lastConversionTime;
    static const unsigned long CONVERSION_DELAY[4];  // Delays for 9-12 bit resolution
};

#endif // DS18B20_H