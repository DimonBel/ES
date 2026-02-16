#include "keypad.h"

Keypad::Keypad(const uint8_t *rowPins, const uint8_t *colPins,
               uint8_t rows, uint8_t cols)
    : _rowPins(rowPins), _colPins(colPins), _rows(rows), _cols(cols),
      _lastKeyTime(0), _lastKey('\0') {}

void Keypad::begin()
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
    fprintf(stderr, "[KEYPAD] Initialized (matrix scan + serial fallback)\n");
}

bool Keypad::isValidKey(char c)
{
    for (uint8_t row = 0; row < 4; row++)
    {
        for (uint8_t col = 0; col < 4; col++)
        {
            if (_keys[row][col] == c)
                return true;
        }
    }
    return false;
}

char Keypad::scanMatrix()
{
    for (uint8_t r = 0; r < _rows; r++)
    {
        // Drive current row LOW
        digitalWrite(_rowPins[r], LOW);

        for (uint8_t c = 0; c < _cols; c++)
        {
            if (digitalRead(_colPins[c]) == LOW)
            {
                // Wait for key release
                while (digitalRead(_colPins[c]) == LOW)
                    ;
                digitalWrite(_rowPins[r], HIGH);
                return _keys[r][c];
            }
        }

        // Restore row HIGH
        digitalWrite(_rowPins[r], HIGH);
    }
    return '\0';
}

char Keypad::getKey()
{
    // 1. Try physical matrix scan
    char key = scanMatrix();

    // 2. Fallback: read from Serial
    if (key == '\0' && Serial.available())
    {
        char c = (char)Serial.read();

        // Convertim litere mici în litere mari pentru A, B, C, D
        if (c >= 'a' && c <= 'd')
            c = c - 'a' + 'A';

        if (isValidKey(c))
            key = c;
    }

    // 3. Debounce
    if (key != '\0')
    {
        unsigned long now = millis();
        if (key == _lastKey && (now - _lastKeyTime) < DEBOUNCE_MS)
            return '\0'; // Ignore bounce
        _lastKey = key;
        _lastKeyTime = now;
    }

    return key;
}

bool Keypad::keyPressed()
{
    return (scanMatrix() != '\0') || (Serial.available() > 0);
}
