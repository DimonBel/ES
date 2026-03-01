#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

class Joystick
{
public:
    Joystick(uint8_t pinX, uint8_t pinY, uint8_t pinSW);

    void begin();
    bool scan();
    bool isPressed();
    bool wasPressed();
    uint16_t getX();
    uint16_t getY();
    int8_t getDirection();
    uint32_t getPressDuration();

    // Direction constants
    static const int8_t DIR_NONE = 0;
    static const int8_t DIR_UP = 1;
    static const int8_t DIR_DOWN = 2;
    static const int8_t DIR_LEFT = 3;
    static const int8_t DIR_RIGHT = 4;
    static const int8_t DIR_CENTER = 5;

private:
    uint8_t _pinX;
    uint8_t _pinY;
    uint8_t _pinSW;

    // Button state
    bool _lastState;
    bool _pressed;
    bool _wasPressed;
    uint32_t _pressStartTime;
    uint32_t _duration;

    // Position values
    uint16_t _x;
    uint16_t _y;

    // Threshold for direction detection (center point ± threshold)
    static const uint16_t CENTER_THRESHOLD = 100;
    static const uint16_t MIN_VALUE = 0;
    static const uint16_t MAX_VALUE = 1023;
    static const uint16_t CENTER = 512;
};

#endif // JOYSTICK_H