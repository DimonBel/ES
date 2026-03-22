#include "actuator.h"

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
    printf("[ACTUATOR] Initialized on pin %d (OFF)\n", _pin);
}

void Actuator::turnOn()
{
    if (_state != STATE_ON)
    {
        digitalWrite(_pin, HIGH);  // Relay ON
        setState(STATE_ON);
        printf("[ACTUATOR] Turned ON (pin %d)\n", _pin);
    }
}

void Actuator::turnOff()
{
    if (_state != STATE_OFF)
    {
        digitalWrite(_pin, LOW);  // Relay OFF
        setState(STATE_OFF);
        printf("[ACTUATOR] Turned OFF (pin %d)\n", _pin);
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
    _state = newState;
    _lastToggleTime = millis();
}