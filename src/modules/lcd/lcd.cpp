#include "lcd.h"
#include <stdarg.h>

LcdI2c::LcdI2c(uint8_t address, uint8_t cols, uint8_t rows)
    : _lcd(nullptr), _cols(cols), _rows(rows) {}

void LcdI2c::begin()
{
    _lcd = new LiquidCrystal_I2C(0x27, _cols, _rows);
    _lcd->init();
    _lcd->backlight();
    _lcd->clear();
    _lcd->setCursor(0, 0);
    printf("[LCD] Hardware initialized at I2C\n");
}

void LcdI2c::print(const char *text)
{
    if (_lcd) _lcd->print(text);
}

void LcdI2c::println(const char *text)
{
    if (_lcd) _lcd->print(text);
}

void LcdI2c::setCursor(uint8_t col, uint8_t row)
{
    if (row >= _rows)
        row = _rows - 1;
    if (col >= _cols)
        col = _cols - 1;
    if (_lcd) _lcd->setCursor(col, row);
}

void LcdI2c::clear()
{
    if (_lcd) _lcd->clear();
}

void LcdI2c::backlight()
{
    if (_lcd) _lcd->backlight();
}

void LcdI2c::noBacklight()
{
    if (_lcd) _lcd->noBacklight();
}

void LcdI2c::write(uint8_t value)
{
    if (_lcd) _lcd->write(value);
}

int LcdI2c::printf(const char *format, ...)
{
    if (!_lcd) return 0;
    
    char buffer[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    _lcd->print(buffer);
    return len;
}
