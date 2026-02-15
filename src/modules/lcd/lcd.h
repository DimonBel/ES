#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// Wrapper pentru LCD I2C 16x2
class LcdI2c
{
public:
    LcdI2c(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2);

    void begin();
    void print(const char *text);
    void println(const char *text);
    void setCursor(uint8_t col, uint8_t row);
    void clear();
    void backlight();
    void noBacklight();
    void write(uint8_t value);

private:
    LiquidCrystal_I2C _lcd;
    uint8_t _cols;
    uint8_t _rows;
};

#endif // LCD_H
