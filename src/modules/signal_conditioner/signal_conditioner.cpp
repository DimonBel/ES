#include "signal_conditioner.h"

SignalConditioner::SignalConditioner()
    : _lastRawState(false),
      _conditionedState(false),
      _pendingState(false),
      _stateChangeTime(0),
      _lastTransitionTime(0),
      _validationStartTime(0),
      _isValidating(false)
{
}

// Saturation function - ensures signal is within valid range
bool SignalConditioner::saturateSignal(int rawValue, int minValue, int maxValue, int *saturatedValue)
{
    if (saturatedValue == nullptr)
    {
        return false;
    }

    if (rawValue < minValue)
    {
        *saturatedValue = minValue;
        // printf("[SIGNAL_COND] Signal saturated: %d -> %d (below min)\n", rawValue, minValue);
        return true;
    }
    else if (rawValue > maxValue)
    {
        *saturatedValue = maxValue;
        // printf("[SIGNAL_COND] Signal saturated: %d -> %d (above max)\n", rawValue, maxValue);
        return true;
    }
    else
    {
        *saturatedValue = rawValue;
        return false;  // No saturation needed
    }
}

// Debouncing function - filters out rapid signal changes
bool SignalConditioner::debounceSignal(bool rawSignal, uint32_t debounceTimeMs)
{
    uint32_t currentTime = millis();

    // If signal changed, start debounce timer
    if (rawSignal != _lastRawState)
    {
        _stateChangeTime = currentTime;
        _lastRawState = rawSignal;
        // printf("[DEBOUNCE] Signal changed to %d, starting debounce...\n", rawSignal);
        return _conditionedState;  // Return previous state during debounce
    }

    // Check if debounce time has elapsed
    if ((currentTime - _stateChangeTime) >= debounceTimeMs)
    {
        // Debounce period complete, update conditioned state
        if (rawSignal != _conditionedState)
        {
            _conditionedState = rawSignal;
            _lastTransitionTime = currentTime;
            // printf("[DEBOUNCE] Debounce complete, new state: %d\n", _conditionedState);
        }
        return _conditionedState;
    }

    // Still in debounce period, return previous state
    return _conditionedState;
}

// Persistent state validation - confirms state stability
bool SignalConditioner::validatePersistentState(bool targetState, uint32_t validationTimeMs)
{
    uint32_t currentTime = millis();

    // If not currently validating, check if we should start
    if (!_isValidating)
    {
        if (_conditionedState == targetState)
        {
            _isValidating = true;
            _validationStartTime = currentTime;
            _pendingState = targetState;
            // printf("[VALIDATE] Starting validation for state %d...\n", targetState);
        }
        return _conditionedState;
    }

    // If target state changed during validation, restart validation
    if (_conditionedState != _pendingState)
    {
        _isValidating = false;
        // printf("[VALIDATE] State changed during validation, restarting...\n");
        return _conditionedState;
    }

    // Check if validation time has elapsed
    if ((currentTime - _validationStartTime) >= validationTimeMs)
    {
        // Validation complete, state is stable
        bool finalState = _pendingState;
        _isValidating = false;
        // printf("[VALIDATE] Validation complete, state confirmed: %d\n", finalState);
        return finalState;
    }

    // Still validating, return current conditioned state
    return _conditionedState;
}

// Combined signal conditioning with all three functions
bool SignalConditioner::conditionSignal(bool rawSignal,
                                        uint32_t debounceTimeMs,
                                        uint32_t validationTimeMs)
{
    // Step 1: Apply debouncing
    bool debouncedState = debounceSignal(rawSignal, debounceTimeMs);

    // Step 2: Validate persistent state (optional, can be skipped if validationTimeMs is 0)
    if (validationTimeMs > 0)
    {
        return validatePersistentState(debouncedState, validationTimeMs);
    }

    return debouncedState;
}

bool SignalConditioner::getConditionedState() const
{
    return _conditionedState;
}

void SignalConditioner::reset()
{
    _lastRawState = false;
    _conditionedState = false;
    _pendingState = false;
    _stateChangeTime = 0;
    _lastTransitionTime = 0;
    _validationStartTime = 0;
    _isValidating = false;
    printf("[SIGNAL_COND] Reset complete\n");
}

uint32_t SignalConditioner::getLastTransitionTime() const
{
    return _lastTransitionTime;
}