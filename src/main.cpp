#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "led.h"

// STDIO helpers to route avr-libc stdio to Arduino Serial

int serial_putchar(char c, FILE* stream) {
  if (c == '\n') Serial.write('\r');
  Serial.write(c);
  return 0;
}

int serial_getchar(FILE* stream) {
  while (!Serial.available());
  int c = Serial.read();
  return c;
}

static FILE serial_stdout;
static FILE serial_stdin;

// instantiate two LEDs (change pins as needed)
static Led led1(12);
static Led led2(13);

void setup() {
  Serial.begin(9600);
  led1.begin();
  led2.begin();

  // attach stdio streams
  fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  fdev_setup_stream(&serial_stdin, NULL, serial_getchar, _FDEV_SETUP_READ);
  stdout = &serial_stdout;
  stdin = &serial_stdin;

  printf("STDIO serial ready. Commands: 'led1 on', 'led1 off', 'led2 on', 'led2 off'\n");
}

#define LINE_BUF 80

static void strip_crlf(char* s) {
  for (char* p = s; *p; ++p) {
    if (*p == '\r' || *p == '\n') { *p = 0; break; }
  }
}

void loop() {
  char line[LINE_BUF];
  if (fgets(line, sizeof(line), stdin) != NULL) {
    strip_crlf(line);
    if (strcmp(line, "led1 on") == 0) {
      led1.on();
      printf("OK: LED1 is ON\n");
    } else if (strcmp(line, "led1 off") == 0) {
      led1.off();
      printf("OK: LED1 is OFF\n");
    } else if (strcmp(line, "led2 on") == 0) {
      led2.on();
      printf("OK: LED2 is ON\n");
    } else if (strcmp(line, "led2 off") == 0) {
      led2.off();
      printf("OK: LED2 is OFF\n");
    } else if (strcmp(line, "led both on") == 0) {
      // Turn on LED1, wait 1 second, then turn on LED2
      led1.on();
      printf("OK: LED1 is ON (starting sequence)\n");
      delay(1000);
      led2.on();
      printf("OK: LED2 is ON (sequence complete)\n");
    } else if (strcmp(line, "led both off") == 0) {
      // Turn both off immediately
      led1.off();
      led2.off();
      printf("OK: Both LEDs are OFF\n");
    } else if (line[0] == '\0') {
      // ignore empty lines
    } else {
      printf("ERR: Unknown command '%s'\n", line);
    }
  }
}