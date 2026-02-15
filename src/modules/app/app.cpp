#include "app.h"

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
    delay(50);
    _lcd.setCursor(0, 0);
    _lcd.print("Enter Code:");
    delay(50);
    _lcd.setCursor(0, 1);
    _lcd.print("____");

    // Stingem LED-urile inițial
    _ledGreen.off();
    _ledRed.off();

    printf("LCD: Welcome screen displayed\n");
}

void App::processInput(char key)
{
    printf("Key pressed: %c\n", key); // Debug STDIO

    // Tasta de ștergere
    if (key == CLEAR_KEY)
    {
        clearInput();
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print("Enter Code:");
        _lcd.setCursor(0, 1);
        _lcd.print("");
        printf("Code cleared\n");
        return;
    }

    // Tasta de confirmare
    if (key == ENTER_KEY)
    {
        if (_inputPos > 0)
        {
            _inputCode[_inputPos] = '\0';
            printf("Code entered: %s\n", _inputCode);

            if (verifyCode())
            {
                handleValidCode();
            }
            else
            {
                handleInvalidCode();
            }

            delay(2000); // Aștept înainte de a resurecta
            displayWelcome();
        }
        return;
    }

    // Cifre și alte caractere valide (0-9, A-D)
    if (isdigit(key) || (key >= 'A' && key <= 'D'))
    {
        if (_inputPos < CODE_LENGTH)
        {
            _inputCode[_inputPos] = key;
            _inputPos++;

            // Afișez asteriscul în loc de caracter pentru securitate
            _lcd.setCursor(_inputPos - 1, 1);
            _lcd.write('*');
            printf("Code position %d: %c\n", _inputPos, key);
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
    printf("Code VALID! Unlocking...\n");

    _lcd.clear();
    _lcd.setCursor(2, 0);
    _lcd.print("Code Valid!");
    _lcd.setCursor(2, 1);
    _lcd.print("Door Unlocked");

    // Aprind LED verde și stingem LED roșu
    _ledGreen.on();
    _ledRed.off();

    delay(500);
    _ledGreen.off();
    delay(200);
    _ledGreen.on();
    delay(500);
    _ledGreen.off();
}

void App::handleInvalidCode()
{
    printf("Code INVALID!\n");

    _lcd.clear();
    _lcd.setCursor(2, 0);
    _lcd.print("Code Invalid!");
    _lcd.setCursor(1, 1);
    _lcd.print("Access Denied");

    // Aprind LED roșu și stingem LED verde
    _ledRed.on();
    _ledGreen.off();

    // Bliț LED roșu
    for (int i = 0; i < 3; i++)
    {
        delay(200);
        _ledRed.off();
        delay(200);
        _ledRed.on();
    }
    _ledRed.off();
}
