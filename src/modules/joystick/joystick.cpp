#include "joystick.h"

Joystick::Joystick(uint8_t pinX, uint8_t pinY, uint8_t pinSW)
    : _pinX(pinX), _pinY(pinY), _pinSW(pinSW),
      _lastState(false), _pressed(false), _wasPressed(false),
      _pressStartTime(0), _duration(0), _x(512), _y(512) {}

void Joystick::begin()
{
    // Analog pins don't need pinMode for input
    // Set button pin as INPUT_PULLUP
    pinMode(_pinSW, INPUT_PULLUP);
}

bool Joystick::scan()
{
    // Read analog values
    _x = analogRead(_pinX);
    _y = analogRead(_pinY);

    // Read button state (LOW when pressed)
    bool curr = (digitalRead(_pinSW) == LOW);

    // Detect rising edge (button pressed)
    if (curr && !_lastState)
    {
        _pressed = true;
        _wasPressed = true;
        _pressStartTime = millis();
    }
    // Detect falling edge (button released)
    else if (!curr && _lastState && _pressed)
    {
        _duration = millis() - _pressStartTime;
        _pressed = false;
    }

    _lastState = curr;
    return _pressed;
}

bool Joystick::isPressed()
{
    return _pressed;
}

bool Joystick::wasPressed()
{
    bool was = _wasPressed;
    _wasPressed = false;
    return was;
}

uint16_t Joystick::getX()
{
    return _x;
}

uint16_t Joystick::getY()
{
    return _y;
}

int8_t Joystick::getDirection()
{
    // Check if joystick is near center
    if (abs((int)_x - CENTER) < CENTER_THRESHOLD &&
        abs((int)_y - CENTER) < CENTER_THRESHOLD)
    {
        return DIR_CENTER;
    }

    // Determine direction based on which axis deviates more
    int deltaX = abs((int)_x - CENTER);
    int deltaY = abs((int)_y - CENTER);

    if (deltaX > deltaY)
    {
        // Horizontal movement
        if (_x > CENTER)
            return DIR_RIGHT;
        else
            return DIR_LEFT;
    }
    else
    {
        // Vertical movement
        if (_y > CENTER)
            return DIR_DOWN;
        else
            return DIR_UP;
    }
}

uint32_t Joystick::getPressDuration()
{
    uint32_t dur = _duration;
    _duration = 0;  // Reset after reading
    return dur;
}