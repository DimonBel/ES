#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include <stdio.h>

class Actuator
{
public:
    // Actuator state enumeration
    enum State {
        STATE_OFF = 0,
        STATE_ON = 1,
        STATE_ERROR = 2
    };

    Actuator(uint8_t pin);

    void begin();
    void turnOn();
    void turnOff();
    void toggle();
    State getState() const;
    bool isActive() const;
    uint32_t getLastToggleTime() const;

    // Get state as string
    const char* getStateString() const;

private:
    uint8_t _pin;
    State _state;
    uint32_t _lastToggleTime;

    void setState(State newState);
};

#endif // ACTUATOR_H