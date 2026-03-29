#ifndef SERIAL_STDIO_H
#define SERIAL_STDIO_H

#include <Arduino.h>
#include <stdio.h>

class SerialStdio {
public:
    // Инициализация драйвера Serial
    static void begin(unsigned long baudRate);

    // Считывание строки из Serial с помощью scanf (игнорирует пробелы и переносы)
    static bool readCommand(char* buffer, size_t bufferSize);

    // Вывод текста в Serial с помощью printf
    static void print(const char* format, ...);
};

#endif // SERIAL_STDIO_H
