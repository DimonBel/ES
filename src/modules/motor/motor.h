#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor {
public:
    enum Direction { BACKWARD = -1, STOP = 0, FORWARD = 1 };

    Motor(uint8_t in1Pin, uint8_t in2Pin, uint8_t enaPin, uint8_t ledcChannel = 1);
    void begin();
    void forward(uint8_t speedPercent);
    void backward(uint8_t speedPercent);
    void stop();
    Direction getDirection() const;
    uint8_t getSpeed() const;
    const char* getDirectionString() const;

private:
    uint8_t _in1, _in2, _ena, _ledcChannel;
    Direction _direction;
    uint8_t _speed;
    void applySpeed(uint8_t speedPercent);
};

#endif
