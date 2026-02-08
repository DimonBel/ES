#include "led.h"

Led::Led(uint8_t pin)
    : _pin(pin), _state(false) {}

void Led::begin()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _state = false;
}

void Led::on()
{
  digitalWrite(_pin, HIGH);
  _state = true;
}

void Led::off()
{
  digitalWrite(_pin, LOW);
  _state = false;
}

void Led::toggle()
{
  if (_state)
    off();
  else
    on();
}

bool Led::state() const { return _state; }
