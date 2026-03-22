#ifndef SIGNAL_CONDITIONER_H
#define SIGNAL_CONDITIONER_H

#include <Arduino.h>
#include <stdio.h>

class SignalConditioner
{
public:
    SignalConditioner();

    // Saturation function - ensures signal is within valid range
    static bool saturateSignal(int rawValue, int minValue, int maxValue, int *saturatedValue);

    // Debouncing function - filters out rapid signal changes
    bool debounceSignal(bool rawSignal, uint32_t debounceTimeMs);

    // Persistent state validation - confirms state stability
    bool validatePersistentState(bool targetState, uint32_t validationTimeMs);

    // Combined signal conditioning with all three functions
    bool conditionSignal(bool rawSignal,
                        uint32_t debounceTimeMs,
                        uint32_t validationTimeMs);

    // Get current conditioned state
    bool getConditionedState() const;

    // Reset internal state
    void reset();

    // Get last transition time
    uint32_t getLastTransitionTime() const;

private:
    bool _lastRawState;
    bool _conditionedState;
    bool _pendingState;
    uint32_t _stateChangeTime;
    uint32_t _lastTransitionTime;
    uint32_t _validationStartTime;
    bool _isValidating;
};

#endif // SIGNAL_CONDITIONER_H