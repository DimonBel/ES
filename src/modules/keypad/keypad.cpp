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
    // Scanare randul por randul
    for (uint8_t row = 0; row < _rows; row++)
    {
        // Activez rândul curent
        digitalWrite(_rowPins[row], LOW);

        delay(DEBOUNCE_DELAY);

        // Citit coloanele
        for (uint8_t col = 0; col < _cols; col++)
        {
            if (digitalRead(_colPins[col]) == LOW)
            {
                delay(DEBOUNCE_DELAY); // Debounce

                // Verific din nou pentru a confirma
                if (digitalRead(_colPins[col]) == LOW)
                {
                    // Aștept apăsare (pentru evitarea repetării)
                    delay(KEY_HOLD_DELAY);

                    // Aștept eliberare
                    while (digitalRead(_colPins[col]) == LOW)
                    {
                        delay(DEBOUNCE_DELAY);
                    }

                    delay(DEBOUNCE_DELAY);

                    // Dezactivez rândul
                    digitalWrite(_rowPins[row], HIGH);
                    return _keys[row][col];
                }
            }
        }

        // Dezactivez rândul
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
