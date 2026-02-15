#include "app.h"
#include <string.h>
#include <ctype.h>

const char App::CORRECT_CODE[] = "1234";

App::App(Keypad &keypad, LcdI2c &lcd, Led &ledGreen, Led &ledRed)
    : _keypad(keypad), _lcd(lcd), _ledGreen(ledGreen), _ledRed(ledRed),
      _inputPos(0)
{
    memset(_inputCode, 0, sizeof(_inputCode));
}

void App::begin()
{
    _keypad.begin();
    _lcd.begin();
    _ledGreen.begin();
    _ledRed.begin();

    displayWelcome();
}

void App::run()
{
    char key = _keypad.getKey();

    if (key != '\0')
    {
        processInput(key);
    }
}

void App::clearInput()
{
    memset(_inputCode, 0, sizeof(_inputCode));
    _inputPos = 0;
}

void App::displayWelcome()
{
    _lcd.clear();
    delay(200);

    _lcd.setCursor(0, 0);
    delay(50);
    _lcd.print("Enter Code:");
    delay(50);

    _lcd.setCursor(0, 1);
    delay(50);
    _lcd.print("[    ]");
    delay(100);

    _ledGreen.off();
    _ledRed.off();

    printf("[APP] Welcome screen ready\n");
}

void App::processInput(char key)
{
    printf("[KEYPAD] %c\n", key);

    if (key == CLEAR_KEY)
    {
        clearInput();
        displayWelcome();
        return;
    }

    if (key == ENTER_KEY)
    {
        if (_inputPos > 0)
        {
            _inputCode[_inputPos] = '\0';
            printf("[CODE] %s\n", _inputCode);

            if (verifyCode())
                handleValidCode();
            else
                handleInvalidCode();

            delay(3000);
            clearInput();
            displayWelcome();
        }
        return;
    }

    if (isdigit(key) || (key >= 'A' && key <= 'D'))
    {
        if (_inputPos < CODE_LENGTH)
        {
            _inputCode[_inputPos] = key;
            _inputPos++;

            _lcd.setCursor(0, 1);
            delay(50);
            _lcd.print("[");
            delay(20);

            for (int i = 0; i < CODE_LENGTH; i++)
            {
                if (i < _inputPos)
                    _lcd.write('*');
                else
                    _lcd.write(' ');
                delay(20);
            }

            _lcd.print("]");
            delay(50);
        }
    }
}

bool App::verifyCode()
{
    // Comparare cod introdus cu codu corect
    return (strcmp(_inputCode, CORRECT_CODE) == 0);
}

void App::handleValidCode()
{
    printf("[SUCCESS] CODE VALID! UNLOCKING!\n");

    _lcd.clear();
    delay(150);

    _lcd.setCursor(0, 0);
    delay(50);
    _lcd.print("ACCESS GRANTED!");
    delay(50);

    _lcd.setCursor(0, 1);
    delay(50);
    _lcd.print("Door Unlocked");
    delay(100);

    _ledGreen.on();
    _ledRed.off();

    for (int i = 0; i < 3; i++)
    {
        delay(300);
        _ledGreen.off();
        delay(200);
        _ledGreen.on();
    }
    _ledGreen.off();
}

void App::handleInvalidCode()
{
    printf("[FAILURE] CODE INVALID! ACCESS DENIED!\n");

    _lcd.clear();
    delay(150);

    _lcd.setCursor(0, 0);
    delay(50);
    _lcd.print("ACCESS DENIED!");
    delay(50);

    _lcd.setCursor(0, 1);
    delay(50);
    _lcd.print("Wrong Code");
    delay(100);

    _ledRed.on();
    _ledGreen.off();

    for (int i = 0; i < 5; i++)
    {
        delay(150);
        _ledRed.off();
        delay(150);
        _ledRed.on();
    }
    _ledRed.off();
}
