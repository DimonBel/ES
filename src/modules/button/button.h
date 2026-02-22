#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <stdio.h>

class Button
{
public:
    Button(const uint8_t *rowPins, const uint8_t *colPins,
           uint8_t rows, uint8_t cols, uint8_t buttonId);

    void begin();
    bool isPressed();
    bool scan();
    uint32_t getPressDuration();
    
private:
    const uint8_t *_rowPins;
    const uint8_t *_colPins;
    uint8_t _rows;
    uint8_t _cols;
    uint8_t _buttonId;
    
    // Button state
    bool _lastState;
    bool _pressed;
    uint32_t _pressStartTime;
    uint32_t _duration;
    
    // Matrix scan
    bool scanMatrix();
};

#endif // BUTTON_H