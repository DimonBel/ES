#include "actuator.h"
#include <Arduino.h>
#include <stdio.h>

Actuator::Actuator(uint8_t pin)
    : _pin(pin), _state(STATE_OFF), _lastToggleTime(0)
{
}

void Actuator::begin()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);  // Initialize relay OFF
    _state = STATE_OFF;
    _lastToggleTime = millis();
    printf("[ACTUATOR] Initialized on pin %d (Binary Relay Mode)\n", _pin);
}

void Actuator::turnOn()
{
    if (_state != STATE_ON)
    {
        _state = STATE_ON;
        _lastToggleTime = millis();
        digitalWrite(_pin, HIGH);  // Relay ON (Full power)
        printf("[ACTUATOR] Relay ON (pin %d)\n", _pin);
    }
}

void Actuator::turnOff()
{
    if (_state != STATE_OFF)
    {
        _state = STATE_OFF;
        _lastToggleTime = millis();
        digitalWrite(_pin, LOW);  // Relay OFF
        printf("[ACTUATOR] Relay OFF (pin %d)\n", _pin);
    }
}

void Actuator::setSpeed(uint8_t percentage)
{
    // A mechanical relay cannot handle PWM speed control.
    // We treat > 0% as ON and 0% as OFF.
    if (percentage > 0) {
        turnOn();
    } else {
        turnOff();
    }
}

void Actuator::toggle()
{
    if (_state == STATE_ON)
    {
        turnOff();
    }
    else
    {
        turnOn();
    }
}

Actuator::State Actuator::getState() const
{
    return _state;
}

bool Actuator::isActive() const
{
    return (_state == STATE_ON);
}

uint32_t Actuator::getLastToggleTime() const
{
    return _lastToggleTime;
}

const char* Actuator::getStateString() const
{
    switch (_state)
    {
        case STATE_ON:
            return "ON";
        case STATE_OFF:
            return "OFF";
        case STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void Actuator::setState(State newState)
{
    if (newState == STATE_ON) {
        turnOn();
    } else {
        turnOff();
    }
}
