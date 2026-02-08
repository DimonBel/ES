#include "serial_stdio.h"
#include <stdio.h>

FILE SerialStdio::serial_stdout;
FILE SerialStdio::serial_stdin;

void SerialStdio::begin(unsigned long baudRate) {
    Serial.begin(baudRate);

    fdev_setup_stream(&serial_stdout, serialPutchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&serial_stdin, NULL, serialGetchar, _FDEV_SETUP_READ);
    stdout = &serial_stdout;
    stdin = &serial_stdin;
}

void SerialStdio::printWelcome() {
    printf("STDIO serial ready. Commands: 'led1 on', 'led1 off', 'led2 on', 'led2 off'\n");
    printf("                           'led both on', 'led both off'\n");
}

int SerialStdio::readLine(char* buffer, int bufferSize) {
    int pos = 0;

    while (true) {
        int c = getchar();

        if (c == '\r' || c == '\n') {
            buffer[pos] = '\0';
            return pos;
        }

        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
            }
            continue;
        }

        if (c >= 32 && c <= 126 && pos < bufferSize - 1) {
            buffer[pos++] = (char)c;
        }
    }
}

int SerialStdio::serialPutchar(char c, FILE* stream) {
    if (c == '\n') Serial.write('\r');
    Serial.write(c);
    return 0;
}

int SerialStdio::serialGetchar(FILE* stream) {
    while (!Serial.available());
    int c = Serial.read();

    if (c == '\b' || c == 127) {
        Serial.write("\b \b");
        return c;
    }

    return c;
}
