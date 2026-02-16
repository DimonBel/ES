#include "lcd.h"

LcdI2c::LcdI2c(uint8_t address, uint8_t cols, uint8_t rows)
    : _lcd(address, cols, rows), _cols(cols), _rows(rows) {}

void LcdI2c::begin()
{
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
    _lcd.setCursor(0, 0);
    fprintf(stderr, "[LCD] Hardware initialized at I2C\n");
}

void LcdI2c::print(const char *text)
{
    _lcd.print(text);
    printf("[LCD] %s", text);
}
void LcdI2c::println(const char *text)
{
    _lcd.print(text);
    printf("[LCD] %s\n", text);
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
    fprintf(stderr, "[LCD] CLEAR\n");
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
