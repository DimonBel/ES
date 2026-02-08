#include <Arduino.h>
#include "serial_stdio/serial_stdio.h"
#include "command/command.h"
#include "led/led.h"

static const unsigned long SERIAL_BAUD_RATE = 9600;
static const uint8_t LED1_PIN = 9;
static const uint8_t LED2_PIN = 13;

static Led led1(LED1_PIN);
static Led led2(LED2_PIN);

static void executeCommand(CommandType cmd)
{
  switch (cmd)
  {
  case CMD_LED1_ON:
    led1.on();
    printf("OK: LED1 is ON\n");
    break;

  case CMD_LED1_OFF:
    led1.off();
    printf("OK: LED1 is OFF\n");
    break;

  case CMD_LED2_ON:
    led2.on();
    printf("OK: LED2 is ON\n");
    break;

  case CMD_LED2_OFF:
    led2.off();
    printf("OK: LED2 is OFF\n");
    break;

  case CMD_BOTH_ON:
    led1.on();
    printf("OK: LED1 is ON (starting sequence)\n");
    delay(500);
    led2.on();
    printf("OK: LED2 is ON (sequence complete)\n");
    break;

  case CMD_BOTH_OFF:
    led1.off();
    led2.off();
    printf("OK: Both LEDs are OFF\n");
    break;

  case CMD_EMPTY:
    break;

  case CMD_UNKNOWN:
  default:
    printf("ERR: Unknown command\n");
    break;
  }
}

void setup()
{
  SerialStdio::begin(SERIAL_BAUD_RATE);

  led1.begin();
  led2.begin();

  SerialStdio::printWelcome();
}

void loop()
{
  char line[SerialStdio::LINE_BUF_SIZE];

  SerialStdio::readLine(line, sizeof(line));

  CommandType cmd = CommandParser::parse(line);

  printf("DEBUG: Received '%s' -> %s\n",
         line,
         CommandParser::toString(cmd));

  executeCommand(cmd);
}
