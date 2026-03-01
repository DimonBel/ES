#ifndef STDIO_REDIRECT_H
#define STDIO_REDIRECT_H

#include <Arduino.h>
#include "../lcd/lcd.h"
#include "../keypad/keypad.h"

// Redirects stdout -> LCD and stdin -> Keypad
// Special characters:
//   \f  – clear screen, cursor to (0,0)
//   \n  – move to next row, column 0
//   \r  – move to column 0 of current row

void stdio_lcd_keypad_init(LcdI2c *lcd, Keypad *keypad);

#endif // STDIO_REDIRECT_H
