#include "rgb_led.h"

RgbLed::RgbLed(uint8_t pinR, uint8_t pinG, uint8_t pinB)
    : _pinR(pinR), _pinG(pinG), _pinB(pinB), _r(0), _g(0), _b(0) {
}

RgbLed::~RgbLed() {
    off();
}

void RgbLed::begin() {
    pinMode(_pinR, OUTPUT);
    pinMode(_pinG, OUTPUT);
    pinMode(_pinB, OUTPUT);
    
    // Initialize to off state
    off();
    
    printf("[RGB LED] Initialized on pins R=%d, G=%d, B=%d\n", _pinR, _pinG, _pinB);
}

void RgbLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    _r = r;
    _g = g;
    _b = b;
    _writePins();
}

void RgbLed::setRed(uint8_t brightness) {
    setColor(brightness, 0, 0);
}

void RgbLed::setGreen(uint8_t brightness) {
    setColor(0, brightness, 0);
}

void RgbLed::setBlue(uint8_t brightness) {
    setColor(0, 0, brightness);
}

void RgbLed::off() {
    setColor(0, 0, 0);
}

void RgbLed::red() {
    setColor(255, 0, 0);
}

void RgbLed::green() {
    setColor(0, 255, 0);
}

void RgbLed::blue() {
    setColor(0, 0, 255);
}

void RgbLed::yellow() {
    setColor(255, 255, 0);
}

void RgbLed::cyan() {
    setColor(0, 255, 255);
}

void RgbLed::magenta() {
    setColor(255, 0, 255);
}

void RgbLed::white() {
    setColor(255, 255, 255);
}

void RgbLed::getColor(uint8_t &r, uint8_t &g, uint8_t &b) const {
    r = _r;
    g = _g;
    b = _b;
}

void RgbLed::_writePins() {
    analogWrite(_pinR, _r);
    analogWrite(_pinG, _g);
    analogWrite(_pinB, _b);
}
