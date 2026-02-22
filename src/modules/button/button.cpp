#include "button.h"

Button::Button(const uint8_t *rowPins, const uint8_t *colPins,
               uint8_t rows, uint8_t cols, uint8_t buttonId)
    : _rowPins(rowPins), _colPins(colPins), _rows(rows), _cols(cols),
      _buttonId(buttonId), _lastState(false), _pressed(false),
      _pressStartTime(0), _duration(0) {}

void Button::begin()
{
    // Configure row pins as OUTPUT (active LOW scan)
    for (uint8_t r = 0; r < _rows; r++)
    {
        pinMode(_rowPins[r], OUTPUT);
        digitalWrite(_rowPins[r], HIGH);
    }
    // Configure column pins as INPUT_PULLUP
    for (uint8_t c = 0; c < _cols; c++)
    {
        pinMode(_colPins[c], INPUT_PULLUP);
    }
}

bool Button::scanMatrix()
{
    for (uint8_t r = 0; r < _rows; r++)
    {
        // Drive current row LOW
        digitalWrite(_rowPins[r], LOW);

        for (uint8_t c = 0; c < _cols; c++)
        {
            if (digitalRead(_colPins[c]) == LOW)
            {
                // Restore row HIGH
                digitalWrite(_rowPins[r], HIGH);
                return true;
            }
        }

        // Restore row HIGH
        digitalWrite(_rowPins[r], HIGH);
    }
    return false;
}

bool Button::scan()
{
    bool curr = scanMatrix();
    
    // Detect rising edge (button pressed)
    if (curr && !_lastState)
    {
        _pressed = true;
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

bool Button::isPressed()
{
    return _pressed;
}

uint32_t Button::getPressDuration()
{
    uint32_t dur = _duration;
    _duration = 0;  // Reset after reading
    return dur;
}