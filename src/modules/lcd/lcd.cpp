#include "lcd.h"

LcdI2c::LcdI2c(uint8_t address, uint8_t cols, uint8_t rows)
    : _lcd(address, cols, rows), _cols(cols), _rows(rows) {}

void LcdI2c::begin()
{
    // Delay lung pentru stabilizare I2C
    delay(500);

    // Inițializare completă a LCD-ului
    _lcd.init();

    // Setează backlight
    _lcd.backlight();
    delay(50);

    // Șterge display complet
    _lcd.clear();
    delay(100);

    // Setează cursor la 0,0
    _lcd.setCursor(0, 0);
    delay(50);

    printf("[LCD] Initialized with backlight ON\n");
}

void LcdI2c::print(const char *text)
{
    _lcd.print(text);
}

void LcdI2c::println(const char *text)
{
    print(text);
    setCursor(0, 1);
}

void LcdI2c::setCursor(uint8_t col, uint8_t row)
{
    if (row >= _rows)
        row = _rows - 1;
    if (col >= _cols)
        col = _cols - 1;
    _lcd.setCursor(col, row);
}

void LcdI2c::clear()
{
    _lcd.clear();
    delay(2);
}

void LcdI2c::backlight()
{
    _lcd.backlight();
}

void LcdI2c::noBacklight()
{
    _lcd.noBacklight();
}

void LcdI2c::write(uint8_t value)
{
    _lcd.write(value);
}
