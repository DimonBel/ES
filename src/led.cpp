#include "led.h"

Led::Led(uint8_t pin)
  : _pin(pin), _state(false) {}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _state = false;
}

void Led::on() {
  digitalWrite(_pin, HIGH);
  _state = true;
}

void Led::off() {
  digitalWrite(_pin, LOW);
  _state = false;
}

void Led::toggle() {
  if (_state) off(); else on();
}

bool Led::state() const { return _state; }
// #include "led.h"

// static uint8_t _ledPin = 13;
// static bool _ledState = false;

// void LedInit(uint8_t pin) {
//   _ledPin = pin;
//   pinMode(_ledPin, OUTPUT);
//   digitalWrite(_ledPin, LOW);
//   _ledState = false;
// }

// void LedOn() {
//   digitalWrite(_ledPin, HIGH);
//   _ledState = true;
// }

// void LedOff() {
//   digitalWrite(_ledPin, LOW);
//   _ledState = false;
// }
