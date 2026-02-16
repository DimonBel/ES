#include "stdio_redirect.h"
#include <stdio.h>

static LcdI2c  *g_lcd    = nullptr;
static Keypad  *g_keypad = nullptr;

// Cursor tracking for LCD output
static uint8_t g_col = 0;
static uint8_t g_row = 0;

static int lcd_putchar(char c, FILE *stream) {
    (void)stream;
    if (!g_lcd) return -1;

    if (c == '\f') {
        g_lcd->clear();
        g_col = 0;
        g_row = 0;
    } else if (c == '\n') {
        g_row = (g_row + 1) % 2;
        g_col = 0;
        g_lcd->setCursor(g_col, g_row);
    } else if (c == '\r') {
        g_col = 0;
        g_lcd->setCursor(g_col, g_row);
    } else {
        if (g_col < 16) {
            g_lcd->setCursor(g_col, g_row);
            g_lcd->write(static_cast<uint8_t>(c));
            g_col++;
        }
    }
    return 0;
}

static int keypad_getchar(FILE *stream) {
    (void)stream;
    if (!g_keypad) return _FDEV_EOF;

    char key = g_keypad->getKey();
    if (key == '\0') return _FDEV_EOF;
    return static_cast<unsigned char>(key);
}

static FILE lcd_stream;
static FILE keypad_stream;

// Keep a copy of the original serial stdout for stderr (debug logging)
static FILE *g_serial_stdout = nullptr;

void stdio_lcd_keypad_init(LcdI2c *lcd, Keypad *keypad) {
    g_lcd    = lcd;
    g_keypad = keypad;

    // Save current stdout (Serial) as stderr for debug output
    g_serial_stdout = stdout;
    stderr = g_serial_stdout;

    fdev_setup_stream(&lcd_stream,    lcd_putchar,    NULL,           _FDEV_SETUP_WRITE);
    fdev_setup_stream(&keypad_stream, NULL,           keypad_getchar, _FDEV_SETUP_READ);

    stdout = &lcd_stream;
    stdin  = &keypad_stream;
}
