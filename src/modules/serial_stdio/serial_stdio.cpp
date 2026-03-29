#include "serial_stdio.h"
#include <stdarg.h>

static char s_cmdBuffer[32];
static size_t s_cmdIndex = 0;

void SerialStdio::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    s_cmdIndex = 0;
    memset(s_cmdBuffer, 0, sizeof(s_cmdBuffer));
}

bool SerialStdio::readCommand(char* buffer, size_t bufferSize) {
    while (Serial.available()) {
        char c = Serial.read();
        
        // Handle newline/carriage return as end of command
        if (c == '\n' || c == '\r') {
            if (s_cmdIndex > 0) {
                s_cmdBuffer[s_cmdIndex] = '\0';
                strncpy(buffer, s_cmdBuffer, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                
                // Reset for next command
                s_cmdIndex = 0;
                return true;
            }
            continue;
        }
        
        // Buffer character if there's space
        if (s_cmdIndex < sizeof(s_cmdBuffer) - 1) {
            // Convert to lowercase
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            s_cmdBuffer[s_cmdIndex++] = c;
            
            // Optional: Echo character back to terminal so user can see what they type
            Serial.print(c);
        }
    }
    
    return false;
}

void SerialStdio::print(const char* format, ...) {
    char loc_buf[128];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return;
    }
    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return;
        }
        vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    
    Serial.print(temp);
    if(temp != loc_buf){
        free(temp);
    }
}
