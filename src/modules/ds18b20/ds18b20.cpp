#include "ds18b20.h"

// Conversion delays for different resolutions (in milliseconds)
const unsigned long DS18B20::CONVERSION_DELAY[4] = {
    94,   // 9 bits  (0.5°C)
    188,  // 10 bits (0.25°C)
    375,  // 11 bits (0.125°C)
    750   // 12 bits (0.0625°C)
};

DS18B20::DS18B20(uint8_t pin) 
    : _pin(pin), _oneWire(nullptr), _deviceFound(false), _resolution(12), _lastConversionTime(0) {
}

DS18B20::~DS18B20() {
    if (_oneWire) {
        delete _oneWire;
        _oneWire = nullptr;
    }
}

void DS18B20::begin() {
    _oneWire = new OneWire(_pin);
    
    // Search for DS18B20 device
    _deviceFound = _oneWire->search(_address);
    
    if (_deviceFound) {
        // Check if device is DS18B20 (family code 0x28)
        if (_address[0] != 0x28) {
            _deviceFound = false;
            printf("[DS18B20] Device found but is not DS18B20 (Family: 0x%02X)\n", _address[0]);
        } else {
            printf("[DS18B20] Device found at address: ");
            for (uint8_t i = 0; i < 8; i++) {
                printf("%02X", _address[i]);
                if (i < 7) printf(":");
            }
            printf("\n");
            
            // Configure resolution
            setResolution(_resolution);
        }
    } else {
        printf("[DS18B20] No device found on pin %d\n", _pin);
    }
    
    _oneWire->reset_search();
}

float DS18B20::readTemperature() {
    if (!_deviceFound) {
        return -127.0f;  // Error value
    }
    
    // Request temperature conversion
    if (!requestTemperature()) {
        return -127.0f;
    }
    
    // Wait for conversion to complete
    delay(CONVERSION_DELAY[_resolution - 9]);
    
    // Read the temperature
    return getTemperature();
}

bool DS18B20::requestTemperature() {
    if (!_deviceFound) {
        return false;
    }
    
    _oneWire->reset();
    _oneWire->select(_address);
    _oneWire->write(0x44);  // Start temperature conversion
    
    _lastConversionTime = millis();
    return true;
}

bool DS18B20::isConversionComplete() {
    if (!_deviceFound) {
        return false;
    }
    
    // DS18B20 signals completion by pulling bus low
    return _oneWire->read_bit() == 1;
}

float DS18B20::getTemperature() {
    if (!_deviceFound) {
        return -127.0f;
    }
    
    uint8_t data[9];
    
    _oneWire->reset();
    _oneWire->select(_address);
    _oneWire->write(0xBE);  // Read scratchpad
    
    // Read 9 bytes of scratchpad
    for (uint8_t i = 0; i < 9; i++) {
        data[i] = _oneWire->read();
    }
    
    // Verify CRC
    uint8_t crc = 0;
    for (uint8_t i = 0; i < 8; i++) {
        crc ^= data[i];
    }
    
    if (crc != data[8]) {
        printf("[DS18B20] CRC error!\n", _address[0]);
        return -127.0f;
    }
    
    // Convert raw data to temperature
    int16_t raw = (data[1] << 8) | data[0];
    
    // Handle negative temperatures
    if (raw & 0x8000) {
        raw = ~raw + 1;
    }
    
    float celsius = raw / 16.0f;
    return celsius;
}

void DS18B20::setResolution(uint8_t resolution) {
    if (resolution < 9) resolution = 9;
    if (resolution > 12) resolution = 12;
    
    _resolution = resolution;
    
    if (!_deviceFound) {
        return;
    }
    
    _oneWire->reset();
    _oneWire->select(_address);
    _oneWire->write(0x4E);  // Write to scratchpad
    
    // Write configuration register (TH, TL, and configuration)
    _oneWire->write(0);     // TH register (user byte 1)
    _oneWire->write(0);     // TL register (user byte 2)
    
    // Configuration register: R1 R0 1 1 1 1 1 1
    // R1 R0: 00=9bit, 01=10bit, 10=11bit, 11=12bit
    uint8_t config = 0x1F | ((resolution - 9) << 5);
    _oneWire->write(config);
    
    printf("[DS18B20] Resolution set to %d bits\n", _resolution);
}

bool DS18B20::isPresent() {
    return _deviceFound;
}