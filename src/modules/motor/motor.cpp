#include "motor/motor.h"

Motor::Motor(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin, uint8_t ledcChannel)
    : _in1(in1Pin), _in2(in2Pin), _ena(enaPin),
      _ledcChannel(ledcChannel), _direction(STOP), _speed(0) {}

void Motor::begin() {
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    ledcSetup(_ledcChannel, 1000, 8);   // 1 kHz, 8-bit resolution
    ledcAttachPin(_ena, _ledcChannel);
    stop();
}

void Motor::applySpeed(uint8_t speedPercent) {
    _speed = speedPercent;
    ledcWrite(_ledcChannel, (uint32_t)(speedPercent * 255UL) / 100);
}

void Motor::forward(uint8_t speedPercent) {
    digitalWrite(_in1, HIGH);
    digitalWrite(_in2, LOW);
    applySpeed(speedPercent);
    _direction = FORWARD;
}

void Motor::backward(uint8_t speedPercent) {
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, HIGH);
    applySpeed(speedPercent);
    _direction = BACKWARD;
}

void Motor::stop() {
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW);
    ledcWrite(_ledcChannel, 0);
    _direction = STOP;
    _speed = 0;
}

Motor::Direction Motor::getDirection() const { return _direction; }
uint8_t Motor::getSpeed() const { return _speed; }

const char* Motor::getDirectionString() const {
    switch (_direction) {
        case FORWARD:  return "FWD";
        case BACKWARD: return "BWD";
        default:       return "STP";
    }
}
