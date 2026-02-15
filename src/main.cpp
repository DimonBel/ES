#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include "serial_stdio/serial_stdio.h"
#include "keypad/keypad.h"
#include "lcd/lcd.h"
#include "led/led.h"
#include "app/app.h"
#include "utils/i2c_scanner.h"

// ========== CONFIGURARE PINI ==========

// Pini pentru Keypad 4x4
static const uint8_t KEYPAD_ROW_PINS[4] = {8, 9, 10, 11}; // R1, R2, R3, R4
static const uint8_t KEYPAD_COL_PINS[4] = {4, 5, 6, 7};   // C1, C2, C3, C4

// Pini pentru LED
static const uint8_t LED_GREEN_PIN = 12;
static const uint8_t LED_RED_PIN = 13;

// Adresa I2C pentru LCD - va fi inițializată din scan
static uint8_t LCD_I2C_ADDRESS = 0x27; // Default, poate fi 0x3F

// Rata baud pentru serial
static const unsigned long SERIAL_BAUD_RATE = 9600;

// ========== OBIECTE GLOBALE ==========

static Keypad keypad(KEYPAD_ROW_PINS, KEYPAD_COL_PINS, 4, 4);
static LcdI2c *lcd_ptr = nullptr; // Va fi inițializat în setup()
static Led ledGreen(LED_GREEN_PIN);
static Led ledRed(LED_RED_PIN);
static App *app_ptr = nullptr; // Va fi inițializat în setup()

// ========== SETUP ==========

void setup()
{
  // Inițializez serial pentru debug/STDIO
  SerialStdio::begin(SERIAL_BAUD_RATE);

  printf("\n\n");
  printf("=====================================\n");
  printf("Electronic Lock System v1.0\n");
  printf("=====================================\n");
  printf("Initializing peripherals...\n\n");

  // 1. TEST LED-urile
  printf("[1/4] Testing LEDs...\n");
  ledGreen.begin();
  ledRed.begin();

  ledGreen.on();
  delay(200);
  ledGreen.off();
  ledRed.on();
  delay(200);
  ledRed.off();
  printf("  LEDs: OK\n");

  // 2. TEST Keypad
  printf("[2/4] Testing Keypad...\n");
  keypad.begin();
  printf("  Keypad: OK (awaiting input)\n");

  // 3. Inițializez I2C
  printf("[3/4] Initializing I2C...\n");
  Wire.begin();
  delay(500);
  printf("  I2C Bus: OK\n");

  // 4. Scann I2C devices
  printf("[4/4] Scanning I2C devices...\n");
  int nDevices = 0;
  for (byte address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0)
    {
      printf("  -> Found device at 0x%02X\n", address);
      nDevices++;
    }
  }

  if (nDevices == 0)
  {
    printf("  WARNING: No I2C devices found!\n");
    printf("  Verify connections:\n");
    printf("    SDA -> A4 | SCL -> A5 | GND | VCC 5V\n");
    LCD_I2C_ADDRESS = 0x27; // Try default anyway
  }
  else
  {
    printf("  Found %d device(s) on I2C\n", nDevices);
  }

  delay(500);

  // Inițializez LCD și App
  printf("\nSetting up LCD at 0x%02X...\n", LCD_I2C_ADDRESS);
  lcd_ptr = new LcdI2c(LCD_I2C_ADDRESS, 16, 2);
  app_ptr = new App(keypad, *lcd_ptr, ledGreen, ledRed);

  printf("Initializing application...\n");
  app_ptr->begin();

  delay(500);

  printf("\n=====================================\n");
  printf("System Ready!\n");
  printf("Code: 1234\n");
  printf("*=Clear, #=Enter\n");
  printf("=====================================\n\n");

  // Test blink LED
  printf("Testing LED blink...\n");
  for (int i = 0; i < 3; i++)
  {
    ledGreen.on();
    delay(100);
    ledGreen.off();
    delay(100);
  }
  printf("Running system loop...\n\n");
}

// ========== LOOP ==========

void loop()
{
  if (app_ptr != nullptr)
  {
    app_ptr->run();
  }
  delay(10);
}
