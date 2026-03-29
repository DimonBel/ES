#include "serial_stdio.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <Arduino.h>
#include "esp_vfs_dev.h"
#include "driver/uart.h"

void SerialStdio::begin(unsigned long baudRate) {
    // Initialize hardware UART via Arduino Serial (easiest way to set up the driver)
    Serial.begin(baudRate);
    
    // This hooks UART0 (the default serial port) into the VFS for standard streams.
    esp_vfs_dev_uart_register();
    esp_vfs_dev_uart_use_driver(UART_NUM_0);
    
    //Disable buffering to ensure input/output is immediate
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

bool SerialStdio::readCommand(char* buffer, size_t bufferSize) {
    // Pure stdio implementation using scanf()
    // This is the standard C library way to read a formatted word.
    // Note: This will block the task calling it (loopTask) until a word is entered.
    // In FreeRTOS, other tasks (Actuator, Servo, Display) continue running normally.
    
    char format[16];
    // Create a safe format string like "%15s" to prevent buffer overflow
    snprintf(format, sizeof(format), "%%%us", (unsigned int)(bufferSize - 1));
    
    // Read one word from stdin
    if (scanf(format, buffer) == 1) {
        // Convert to lowercase using standard C tolower()
        for (size_t i = 0; buffer[i] != '\0'; i++) {
            buffer[i] = tolower((unsigned char)buffer[i]);
        }
        return true;
    }
    
    return false;
}

void SerialStdio::print(const char* format, ...) {
    // Pure stdio implementation using vprintf()
    va_list arg;
    va_start(arg, format);
    vprintf(format, arg);
    va_end(arg);
}
