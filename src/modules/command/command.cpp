#include "command.h"
#include <string.h>
#include <ctype.h>

static void trimWhitespace(char* str) {
    char* end;

    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) {
        *str = 0;
        return;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = 0;
}

static void toLowerCase(char* str) {
    for (; *str; ++str) {
        *str = tolower((unsigned char)*str);
    }
}

CommandType CommandParser::parse(const char* input) {
    if (input == nullptr || input[0] == '\0') {
        return CMD_EMPTY;
    }

    char buffer[MAX_CMD_LENGTH];
    strncpy(buffer, input, MAX_CMD_LENGTH - 1);
    buffer[MAX_CMD_LENGTH - 1] = '\0';

    trimWhitespace(buffer);
    toLowerCase(buffer);

    if (strcmp(buffer, "led1 on") == 0) {
        return CMD_LED1_ON;
    } else if (strcmp(buffer, "led1 off") == 0) {
        return CMD_LED1_OFF;
    } else if (strcmp(buffer, "led2 on") == 0) {
        return CMD_LED2_ON;
    } else if (strcmp(buffer, "led2 off") == 0) {
        return CMD_LED2_OFF;
    } else if (strcmp(buffer, "led both on") == 0) {
        return CMD_BOTH_ON;
    } else if (strcmp(buffer, "led both off") == 0) {
        return CMD_BOTH_OFF;
    }

    return CMD_UNKNOWN;
}

bool CommandParser::isValid(CommandType cmd) {
    return (cmd >= CMD_LED1_ON && cmd <= CMD_BOTH_OFF) || cmd == CMD_EMPTY;
}

const char* CommandParser::toString(CommandType cmd) {
    switch (cmd) {
        case CMD_LED1_ON:      return "led1 on";
        case CMD_LED1_OFF:     return "led1 off";
        case CMD_LED2_ON:      return "led2 on";
        case CMD_LED2_OFF:     return "led2 off";
        case CMD_BOTH_ON:      return "led both on";
        case CMD_BOTH_OFF:     return "led both off";
        case CMD_EMPTY:        return "(empty)";
        case CMD_UNKNOWN:
        default:               return "unknown";
    }
}
