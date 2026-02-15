#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "../keypad/keypad.h"
#include "../lcd/lcd.h"
#include "../led/led.h"

// Configurare aplicație
class App
{
public:
    App(Keypad &keypad, LcdI2c &lcd, Led &ledGreen, Led &ledRed);

    void begin();
    void run();

private:
    Keypad &_keypad;
    LcdI2c &_lcd;
    Led &_ledGreen;
    Led &_ledRed;

    // Codul corect (implicit: 1234)
    static const char CORRECT_CODE[];
    static const int CODE_LENGTH = 4;
    static const char CLEAR_KEY = '*';
    static const char ENTER_KEY = '#';

    char _inputCode[CODE_LENGTH + 1];
    int _inputPos;

    void clearInput();
    void displayWelcome();
    void processInput(char key);
    bool verifyCode();
    void handleValidCode();
    void handleInvalidCode();
};

#endif // APP_H
