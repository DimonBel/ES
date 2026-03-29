#include "servo.h"
#include <stdio.h>
#include "esp32-hal-ledc.h"

// PWM configuration for standard servos
const uint16_t SERVO_PULSE_MIN_US = 500;    // 0.5ms pulse (0°)
const uint16_t SERVO_PULSE_MAX_US = 2500;   // 2.5ms pulse (180°)

// LEDC configuration for Servo
const uint8_t SERVO_LEDC_CHANNEL = 0;       // Use Channel 0 for Servo
const uint32_t LEDC_FREQ = 50;              // 50Hz for servos
const uint8_t LEDC_RESOLUTION = 16;         // 16-bit resolution

Servo::Servo(uint8_t pin)
    : _pin(pin)
    , _currentAngle(90)
    , _currentSpeed(50)
    , _targetAngle(90)
    , _rampSpeed(1)
    , _state(STATE_STOPPED)
    , _initialized(false)
    , _lastUpdateTime(0)
{
}

void Servo::begin() {
    if (_initialized) return;

    // Compatible ESP32 PWM initialization
    ledcSetup(SERVO_LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(_pin, SERVO_LEDC_CHANNEL);

    // Set initial position (center)
    _currentAngle = 90;
    _targetAngle = 90;

    // Apply initial position
    uint16_t pulseWidth = angleToPulseWidth(_currentAngle);
    // 50Hz = 20,000us period. Duty = (pulseWidth / 20000) * 65535
    uint32_t duty = (uint32_t)(((uint64_t)pulseWidth * 65535) / 20000);
    ledcWrite(SERVO_LEDC_CHANNEL, duty);

    _state = STATE_STOPPED;
    _initialized = true;
    _lastUpdateTime = millis();

    printf("[SERVO] Initialized on pin %d (Channel 0, 50Hz)\n", _pin);
}

void Servo::setAngle(uint8_t angle) {
    if (!_initialized) return;
    if (angle > 180) angle = 180;

    _targetAngle = angle;
    uint16_t pulseWidth = angleToPulseWidth(angle);
    uint32_t duty = (uint32_t)(((uint64_t)pulseWidth * 65535) / 20000);
    ledcWrite(SERVO_LEDC_CHANNEL, duty);
    
    _currentAngle = angle;
    _state = STATE_STOPPED;
}

void Servo::setSpeed(uint8_t speed) {
    setAngle(speedToAngle(speed));
}

void Servo::moveTo(uint8_t targetAngle, uint8_t rampSpeed) {
    if (!_initialized) return;
    if (targetAngle > 180) targetAngle = 180;
    
    _targetAngle = targetAngle;
    _rampSpeed = rampSpeed;
    _state = STATE_MOVING;
}

void Servo::stop() {
    if (!_initialized) return;
    ledcWrite(SERVO_LEDC_CHANNEL, 0);
    _state = STATE_STOPPED;
}

void Servo::update() {
    if (!_initialized || _state == STATE_STOPPED) return;

    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime < 20) return;
    _lastUpdateTime = currentTime;

    if (_currentAngle == _targetAngle) {
        _state = STATE_STOPPED;
        return;
    }

    // Move towards target
    if (_targetAngle > _currentAngle) {
        _currentAngle = (_targetAngle - _currentAngle > _rampSpeed) ? _currentAngle + _rampSpeed : _targetAngle;
    } else {
        _currentAngle = (_currentAngle - _targetAngle > _rampSpeed) ? _currentAngle - _rampSpeed : _targetAngle;
    }

    uint16_t pulseWidth = angleToPulseWidth(_currentAngle);
    uint32_t duty = (uint32_t)(((uint64_t)pulseWidth * 65535) / 20000);
    ledcWrite(SERVO_LEDC_CHANNEL, duty);

    if (_currentAngle == _targetAngle) _state = STATE_STOPPED;
}

const char* Servo::getStateString() const {
    return (_state == STATE_MOVING) ? "MOVING" : "STOPPED";
}

uint16_t Servo::angleToPulseWidth(uint8_t angle) const {
    return SERVO_PULSE_MIN_US + (angle * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / 180;
}

uint8_t Servo::speedToAngle(uint8_t speed) const {
    return (speed * 180) / 100;
}

void Servo::updateState() {
    _state = (_currentAngle == _targetAngle) ? STATE_STOPPED : STATE_MOVING;
}
