#include "keypad.h"

Keypad::Keypad(const uint8_t *rowPins, const uint8_t *colPins,
               uint8_t rows, uint8_t cols)
    : _rowPins(rowPins), _colPins(colPins), _rows(rows), _cols(cols) {}

void Keypad::begin()
{
    // Configurare pini randuri (OUTPUT)
    for (uint8_t i = 0; i < _rows; i++)
    {
        pinMode(_rowPins[i], OUTPUT);
        digitalWrite(_rowPins[i], HIGH);
    }

    // Configurare pini coloane (INPUT_PULLUP)
    for (uint8_t i = 0; i < _cols; i++)
    {
        pinMode(_colPins[i], INPUT_PULLUP);
    }
}

char Keypad::getKey()
{
    // Scanare rând cu rând
    for (uint8_t row = 0; row < _rows; row++)
    {
        // Activez rândul curent (LOW)
        digitalWrite(_rowPins[row], LOW);
        delayMicroseconds(100);

        // Citit coloanele
        for (uint8_t col = 0; col < _cols; col++)
        {
            if (digitalRead(_colPins[col]) == LOW)
            {
                // Debounce - aștept 20ms
                delay(20);

                // Verific din nou
                if (digitalRead(_colPins[col]) == LOW)
                {
                    char pressedKey = _keys[row][col];

                    // Aștept apăsare și eliberare (până coloana devine HIGH)
                    while (digitalRead(_colPins[col]) == LOW)
                    {
                        delay(10);
                    }

                    // Debounce pe eliberare
                    delay(50);

                    // Dezactivez rândul
                    digitalWrite(_rowPins[row], HIGH);

                    return pressedKey;
                }
            }
        }

        // Dezactivez rândul (HIGH)
        digitalWrite(_rowPins[row], HIGH);
    }

    return '\0'; // Nici o tastă apăsată
}

bool Keypad::keyPressed()
{
    for (uint8_t row = 0; row < _rows; row++)
    {
        digitalWrite(_rowPins[row], LOW);

        for (uint8_t col = 0; col < _cols; col++)
        {
            if (digitalRead(_colPins[col]) == LOW)
            {
                digitalWrite(_rowPins[row], HIGH);
                return true;
            }
        }

        digitalWrite(_rowPins[row], HIGH);
    }

    return false;
}
