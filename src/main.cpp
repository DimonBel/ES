#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "led.h"

// STDIO helpers to route avr-libc stdio to Arduino Serial

int serial_putchar(char c, FILE *stream)
{
  if (c == '\n')
    Serial.write('\r');
  Serial.write(c);
  return 0;
}

int serial_getchar(FILE *stream)
{
  while (!Serial.available())
    ;
  int c = Serial.read();

  // Handle backspace character (ASCII 8 or 127)
  if (c == '\b' || c == 127)
  {
    // Echo backspace sequence to erase character from terminal
    Serial.write("\b \b");
    return c;
  }

  return c;
}

static FILE serial_stdout;
static FILE serial_stdin;

// instantiate two LEDs (change pins as needed)
static Led led1(9);
static Led led2(13);

void setup()
{
  // Initialize Serial first, then initialize LEDs via the Led API.
  // Avoid calling pinMode/digitalWrite on the same pin here because
  // `led1.begin()` will set the pin mode and initial state.
  Serial.begin(9600);
  led1.begin();
  led2.begin();

  // If you want LED1 to start ON, use `led1.on();` here.

  // attach stdio streams
  fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  fdev_setup_stream(&serial_stdin, NULL, serial_getchar, _FDEV_SETUP_READ);
  stdout = &serial_stdout;
  stdin = &serial_stdin;

  printf("STDIO serial ready. Commands: 'led1 on', 'led1 off', 'led2 on', 'led2 off'\n");
}

#define LINE_BUF 80

static void strip_crlf(char *s)
{
  for (char *p = s; *p; ++p)
  {
    if (*p == '\r' || *p == '\n')
    {
      *p = 0;
      break;
    }
  }
}

static void trim_whitespace(char *s)
{
  char *end;

  // Trim leading space
  while (*s == ' ')
    s++;

  // All spaces?
  if (*s == 0)
  {
    *s = 0;
    return;
  }

  // Trim trailing space
  end = s + strlen(s) - 1;
  while (end > s && *end == ' ')
    end--;

  // Write new null terminator
  *(end + 1) = 0;
}

void loop()
{
  char line[LINE_BUF];
  int pos = 0;

  // Read characters one by one to handle backspace properly
  while (true)
  {
    int c = getchar();

    // Handle Enter key (newline or carriage return)
    if (c == '\r' || c == '\n')
    {
      line[pos] = '\0';
      break;
    }

    // Handle backspace (ASCII 8) or delete (ASCII 127)
    if (c == '\b' || c == 127)
    {
      if (pos > 0)
      {
        pos--;
      }
      continue;
    }

    // Handle printable characters
    if (c >= 32 && c <= 126 && pos < LINE_BUF - 1)
    {
      line[pos++] = (char)c;
    }
  }

  trim_whitespace(line);

  if (strcmp(line, "led1 on") == 0)
  {
    led1.on();
    printf("OK: LED1 is ON\n");
  }
  else if (strcmp(line, "led1 off") == 0)
  {
    led1.off();
    printf("OK: LED1 is OFF\n");
  }
  else if (strcmp(line, "led2 on") == 0)
  {
    led2.on();
    printf("OK: LED2 is ON\n");
  }
  else if (strcmp(line, "led2 off") == 0)
  {
    led2.off();
    printf("OK: LED2 is OFF\n");
  }
  else if (strcmp(line, "led both on") == 0)
  {
    // Turn on LED1, wait 1 second, then turn on LED2
    led1.on();
    printf("OK: LED1 is ON (starting sequence)\n");
    delay(500);
    led2.on();
    printf("OK: LED2 is ON (sequence complete)\n");
  }
  else if (strcmp(line, "led both off") == 0)
  {
    // Turn both off immediately
    led1.off();
    led2.off();
    printf("OK: Both LEDs are OFF\n");
  }
  else if (line[0] == '\0')
  {
    // ignore empty lines
  }
  else
  {
    printf("ERR: Unknown command '%s'\n", line);
  }
}