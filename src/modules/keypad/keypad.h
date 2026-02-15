#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>

class Keypad
{
public:
    Keypad(const uint8_t *rowPins, const uint8_t *colPins,
           uint8_t rows, uint8_t cols);

    void begin();
    char getKey();
    bool keyPressed();

private:
    const uint8_t *_rowPins;
    const uint8_t *_colPins;
    uint8_t _rows;
    uint8_t _cols;

    // Harta tastelor pentru keypad 4x4
    const char _keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};

    static const unsigned long DEBOUNCE_DELAY = 20;  // ms
    static const unsigned long KEY_HOLD_DELAY = 100; // ms
};

#endif // KEYPAD_H
