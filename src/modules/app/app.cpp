#include "app.h"
#include <string.h>
#include <ctype.h>

App::App(Keypad &keypad, LcdI2c &lcd, Led &ledGreen, Led &ledRed, Led &ledProgramming)
    : _keypad(keypad), _lcd(lcd), _ledGreen(ledGreen), _ledRed(ledRed),
      _ledProgramming(ledProgramming),
      _inputPos(0), _programmingMode(false), _newCodePos(0)
{
    memset(_inputCode, 0, sizeof(_inputCode));
    memset(_newCode, 0, sizeof(_newCode));
    strcpy(_correctCode, "1234"); // Default password
}

void App::begin()
{
    _keypad.begin();
    _lcd.begin();
    _ledGreen.begin();
    _ledRed.begin();
    _ledProgramming.begin();

    displayWelcome();
}

void App::run()
{
    char key = _keypad.getKey();

    if (key != '\0')
    {
        if (_programmingMode)
        {
            processProgrammingInput(key);
        }
        else
        {
            processInput(key);
        }
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

    fprintf(stderr, "[APP] Welcome screen ready\n");
}

void App::processInput(char key)
{
    fprintf(stderr, "[KEYPAD] %c\n", key);

    // Enter programming mode with D at welcome screen
    if (key == PROGRAM_KEY && _inputPos == 0)
    {
        enterProgrammingMode();
        return;
    }

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
            fprintf(stderr, "[CODE] %s\n", _inputCode);

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

    if (isdigit(key) || (key >= 'A' && key <= 'C'))
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
    // Comparare cod introdus cu codul corect (variabil)
    return (strcmp(_inputCode, _correctCode) == 0);
}

void App::handleValidCode()
{
    fprintf(stderr, "[SUCCESS] CODE VALID! UNLOCKING!\n");
    fprintf(stderr, "[DEBUG] >>> TURNING ON GREEN LED <<<\n");

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

    fprintf(stderr, "[DEBUG] Before ON - LED state: %d\n", _ledGreen.state());

    _ledGreen.on();

    fprintf(stderr, "[DEBUG] After ON - LED state: %d\n", _ledGreen.state());
    fprintf(stderr, "[DEBUG] LED should be ON for 5 seconds...\n");

    _ledRed.off();

    // Keep LED ON for 5 seconds
    delay(5000);

    fprintf(stderr, "[DEBUG] Turning OFF LED\n");
    _ledGreen.off();
}

void App::handleInvalidCode()
{
    fprintf(stderr, "[FAILURE] CODE INVALID! ACCESS DENIED!\n");

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
void App::enterProgrammingMode()
{
    fprintf(stderr, "[PROGRAMMING] Entered programming mode\n");
    fprintf(stderr, "[PROGRAMMING] Turning ON LED on pin 3\n");

    _programmingMode = true;
    _newCodePos = 0;
    memset(_newCode, 0, sizeof(_newCode));

    _ledProgramming.on();

    _lcd.clear();
    delay(200);

    _lcd.setCursor(0, 0);
    delay(50);
    _lcd.print("PROG MODE");
    delay(50);

    _lcd.setCursor(0, 1);
    delay(50);
    _lcd.print("New Pass [    ]");
    delay(100);
}

void App::processProgrammingInput(char key)
{
    fprintf(stderr, "[PROG-INPUT] %c\n", key);

    // Cancel programming with *
    if (key == CLEAR_KEY)
    {
        fprintf(stderr, "[PROGRAMMING] Cancelled - Turning OFF LED\n");
        _programmingMode = false;
        _newCodePos = 0;
        memset(_newCode, 0, sizeof(_newCode));
        _ledProgramming.off();
        displayWelcome();
        return;
    }

    // Confirm new password with #
    if (key == ENTER_KEY)
    {
        if (_newCodePos == CODE_LENGTH)
        {
            _newCode[_newCodePos] = '\0';
            confirmNewCode();
        }
        else
        {
            fprintf(stderr, "[PROGRAMMING] Need %d digits\n", CODE_LENGTH);
        }
        return;
    }

    // Accept only digits
    if (isdigit(key))
    {
        if (_newCodePos < CODE_LENGTH)
        {
            _newCode[_newCodePos] = key;
            _newCodePos++;

            _lcd.setCursor(0, 1);
            delay(50);
            _lcd.print("New Pass [");
            delay(20);

            for (int i = 0; i < CODE_LENGTH; i++)
            {
                if (i < _newCodePos)
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

void App::confirmNewCode()
{
    fprintf(stderr, "[PROGRAMMING] New password set to: %s\n", _newCode);
    fprintf(stderr, "[PROGRAMMING] Turning OFF LED - exiting programming mode\n");

    strcpy(_correctCode, _newCode);

    _lcd.clear();
    delay(200);

    _lcd.setCursor(0, 0);
    delay(50);
    _lcd.print("PASSWORD SAVED");
    delay(50);

    _lcd.setCursor(0, 1);
    delay(50);
    _lcd.print("New: ");
    _lcd.print(_newCode);
    delay(100);

    _ledGreen.on();
    delay(300);
    _ledGreen.off();
    delay(200);
    _ledGreen.on();
    delay(300);
    _ledGreen.off();

    delay(2000);

    _programmingMode = false;
    _newCodePos = 0;
    memset(_newCode, 0, sizeof(_newCode));
    _ledProgramming.off();
    displayWelcome();
}