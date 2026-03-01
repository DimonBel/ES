#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>

enum CommandType {
    CMD_LED1_ON,
    CMD_LED1_OFF,
    CMD_LED2_ON,
    CMD_LED2_OFF,
    CMD_BOTH_ON,
    CMD_BOTH_OFF,
    CMD_UNKNOWN,
    CMD_EMPTY
};

class CommandParser {
public:
    static CommandType parse(const char* input);

    static bool isValid(CommandType cmd);

    static const char* toString(CommandType cmd);

private:
    static const int MAX_CMD_LENGTH = 32;
};

#endif // COMMAND_H
