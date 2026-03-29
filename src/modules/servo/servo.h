#ifndef SERVO_H
#define SERVO_H

#include <Arduino.h>

/**
 * @class Servo
 * @brief Analog actuator control using PWM for servo motors
 * 
 * Features:
 * - PWM control (50Hz, 1-2ms pulse width)
 * - Angle control (0-180 degrees)
 * - Speed/position control (0-100%)
 * - Smooth transitions with configurable speed
 * - Ramping for actuator protection
 * - State tracking and validation
 */
class Servo {
public:
    // State enumeration
    enum State {
        STATE_STOPPED = 0,
        STATE_MOVING = 1,
        STATE_ERROR = 2
    };

    /**
     * @brief Constructor
     * @param pin GPIO pin for PWM signal
     */
    explicit Servo(uint8_t pin);

    /**
     * @brief Initialize servo
     */
    void begin();

    /**
     * @brief Set servo angle (0-180 degrees)
     * @param angle Target angle in degrees
     */
    void setAngle(uint8_t angle);

    /**
     * @brief Set servo speed as percentage (0-100%)
     * Maps to angle: 0% -> 0°, 100% -> 180°
     * @param speed Speed percentage (0-100)
     */
    void setSpeed(uint8_t speed);

    /**
     * @brief Move servo to position with ramping
     * @param targetAngle Target angle (0-180)
     * @param rampSpeed Speed of ramping (degrees per step)
     */
    void moveTo(uint8_t targetAngle, uint8_t rampSpeed = 1);

    /**
     * @brief Stop servo (detach PWM)
     */
    void stop();

    /**
     * @brief Get current angle
     * @return Current angle in degrees (0-180)
     */
    uint8_t getAngle() const { return _currentAngle; }

    /**
     * @brief Get current speed percentage
     * @return Speed percentage (0-100)
     */
    uint8_t getSpeed() const { return _currentSpeed; }

    /**
     * @brief Get servo state
     * @return Current state
     */
    State getState() const { return _state; }

    /**
     * @brief Get state as string
     * @return State string representation
     */
    const char* getStateString() const;

    /**
     * @brief Check if servo is moving
     * @return true if servo is currently moving
     */
    bool isMoving() const { return _state == STATE_MOVING; }

    /**
     * @brief Update servo (call periodically for smooth movement)
     */
    void update();

private:
    uint8_t _pin;              // PWM pin
    uint8_t _currentAngle;     // Current angle (0-180)
    uint8_t _currentSpeed;     // Current speed percentage (0-100)
    uint8_t _targetAngle;      // Target angle for ramping
    uint8_t _rampSpeed;        // Ramping speed (degrees per step)
    State _state;              // Current state
    bool _initialized;         // Initialization flag
    uint32_t _lastUpdateTime;  // Last update time

    /**
     * @brief Convert angle to microseconds pulse width
     * @param angle Angle in degrees (0-180)
     * @return Pulse width in microseconds (1000-2000)
     */
    uint16_t angleToPulseWidth(uint8_t angle) const;

    /**
     * @brief Convert speed percentage to angle
     * @param speed Speed percentage (0-100)
     * @return Angle in degrees (0-180)
     */
    uint8_t speedToAngle(uint8_t speed) const;

    /**
     * @brief Update state based on movement
     */
    void updateState();
};

#endif // SERVO_H