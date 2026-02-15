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
    App(Keypad &keypad, LcdI2c &lcd, Led &ledGreen, Led &ledRed, Led &ledProgramming);

    void begin();
    void run();

private:
    Keypad &_keypad;
    LcdI2c &_lcd;
    Led &_ledGreen;
    Led &_ledRed;
    Led &_ledProgramming; // LED for programming mode indication

    // Codul corect (implicit: 1234)
    char _correctCode[5]; // Now variable, not const
    static const int CODE_LENGTH = 4;
    static const char CLEAR_KEY = '*';
    static const char ENTER_KEY = '#';
    static const char PROGRAM_KEY = 'D';

    char _inputCode[CODE_LENGTH + 1];
    int _inputPos;
    bool _programmingMode;
    char _newCode[CODE_LENGTH + 1];
    int _newCodePos;

    void clearInput();
    void displayWelcome();
    void processInput(char key);
    bool verifyCode();
    void handleValidCode();
    void handleInvalidCode();
    void enterProgrammingMode();
    void processProgrammingInput(char key);
    void confirmNewCode();
};

#endif // APP_H
