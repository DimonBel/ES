#include "dht11.h"
#include <stdio.h>

Dht11Sensor::Dht11Sensor(uint8_t pin)
    : _pin(pin), _dht(nullptr), _present(false) {}

Dht11Sensor::~Dht11Sensor() {
    delete _dht;
}

void Dht11Sensor::begin() {
    _dht = new DHT(_pin, DHT11);
    _dht->begin();
    _present = true;
    printf("[DHT11] Initialized on GPIO %d\n", _pin);
}

float Dht11Sensor::readTemperature() {
    if (!_dht) return -127.0f;
    float t = _dht->readTemperature();
    return isnan(t) ? -127.0f : t;
}

float Dht11Sensor::readHumidity() {
    if (!_dht) return -1.0f;
    float h = _dht->readHumidity();
    return isnan(h) ? -1.0f : h;
}

bool Dht11Sensor::isPresent() const {
    return _present;
}
