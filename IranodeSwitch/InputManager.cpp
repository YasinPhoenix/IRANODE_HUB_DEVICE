#include "InputManager.h"

void InputManager::begin() {
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        pinMode(TOUCH_PINS[i], INPUT_PULLUP);
        _lastState[i] = false;
        _lastFireMs[i] = 0;
    }
}

bool InputManager::poll(uint8_t &outIndex) {
    uint32_t now = millis();
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        bool pressed = digitalRead(TOUCH_PINS[i]) == LOW;
        bool firePress = pressed && !_lastState[i] && (now - _lastFireMs[i] > TOUCH_DEBOUNCE_MS);
        _lastState[i] = pressed;
        if (firePress) {
            _lastFireMs[i] = now;
            outIndex = i;
            return true;
        }
    }
    return false;
}

bool InputManager::checkFactoryReset(uint32_t nowMs) {
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        if (digitalRead(TOUCH_PINS[i]) != LOW) {
            _allHeldSinceMs = 0; // not everyone is pressed - reset the timer
            return false;
        }
    }
    if (_allHeldSinceMs == 0) {
        _allHeldSinceMs = nowMs; // just became all-held - start timing
        return false;
    }
    return (nowMs - _allHeldSinceMs) >= FACTORY_RESET_HOLD_MS;
}
