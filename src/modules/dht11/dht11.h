#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

class Dht11Sensor {
public:
    explicit Dht11Sensor(uint8_t pin);
    ~Dht11Sensor();

    void  begin();
    float readTemperature();
    float readHumidity();
    bool  isPresent() const;

private:
    uint8_t _pin;
    DHT    *_dht;
    bool    _present;
};

#endif
