#ifndef SERIAL_STDIO_H
#define SERIAL_STDIO_H

#include <Arduino.h>

class SerialStdio {
public:
    static void begin(unsigned long baudRate);

    static void printWelcome();

    static int readLine(char* buffer, int bufferSize);

public:
    static const int LINE_BUF_SIZE = 80;

private:
    static int serialPutchar(char c, FILE* stream);

    static int serialGetchar(FILE* stream);

    static FILE serial_stdout;
    static FILE serial_stdin;
};

#endif // SERIAL_STDIO_H
