#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <stdio.h>

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

    // Caractere valide acceptate de la stdin
    bool isValidKey(char c);

    // Debounce
    unsigned long _lastKeyTime;
    char _lastKey;
    static const unsigned long DEBOUNCE_MS = 200;

    // Scan physical matrix
    char scanMatrix();
};

#endif // KEYPAD_H
