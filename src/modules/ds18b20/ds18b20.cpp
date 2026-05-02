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

    // Try up to 3 times with a short delay between attempts
    for (uint8_t attempt = 1; attempt <= 3; attempt++) {
        _oneWire->reset_search();
        _deviceFound = _oneWire->search(_address);

        if (_deviceFound) {
            if (_address[0] != 0x28) {
                printf("[DS18B20] Device found but not DS18B20 (Family: 0x%02X)\n", _address[0]);
                _deviceFound = false;
            } else {
                printf("[DS18B20] Found on attempt %d, address: ", attempt);
                for (uint8_t i = 0; i < 8; i++) {
                    printf("%02X", _address[i]);
                    if (i < 7) printf(":");
                }
                printf("\n");
                setResolution(_resolution);
                break;
            }
        } else {
            printf("[DS18B20] Attempt %d: no device on pin %d\n", attempt, _pin);
            delay(100);
        }
    }
}

float DS18B20::readTemperature() {
    // If device wasn't found at begin(), try to find it again
    if (!_deviceFound) {
        printf("[DS18B20] Rescanning bus on pin %d...\n", _pin);
        _oneWire->reset_search();
        _deviceFound = _oneWire->search(_address);
        if (_deviceFound && _address[0] != 0x28) {
            printf("[DS18B20] Found device is not DS18B20 (0x%02X)\n", _address[0]);
            _deviceFound = false;
        }
        if (_deviceFound) {
            printf("[DS18B20] Device found on rescan!\n");
        } else {
            printf("[DS18B20] Still not found. Check wiring: DATA->GPIO%d, 4.7k to 3.3V\n", _pin);
            return -127.0f;
        }
    }

    if (!requestTemperature()) {
        return -127.0f;
    }

    delay(CONVERSION_DELAY[_resolution - 9]);
    return getTemperature();
}

bool DS18B20::requestTemperature() {
    if (!_deviceFound) {
        return false;
    }

    if (!_oneWire->reset()) {
        printf("[DS18B20] No presence pulse on reset\n");
        return false;
    }
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
    
    // CRC8 Dallas/Maxim (NOT simple XOR)
    if (OneWire::crc8(data, 8) != data[8]) {
        printf("[DS18B20] CRC error!\n");
        return -127.0f;
    }

    // int16_t is already two's complement – handles negatives correctly
    int16_t raw = (int16_t)(((uint16_t)data[1] << 8) | data[0]);
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