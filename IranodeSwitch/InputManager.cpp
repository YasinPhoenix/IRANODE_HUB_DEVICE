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

bool InputManager::allCurrentlyHeld() const {
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        if (digitalRead(TOUCH_PINS[i]) != LOW) return false;
    }
    return true;
}

uint32_t InputManager::updateHoldTimer(uint32_t nowMs) {
    if (!allCurrentlyHeld()) {
        _allHeldSinceMs = 0; // not everyone is pressed - reset for the next hold
        _consumedForThisHold = false;
        return 0;
    }
    if (_allHeldSinceMs == 0) {
        _allHeldSinceMs = nowMs; // just became all-held - start timing
        return 0;
    }
    return nowMs - _allHeldSinceMs;
}

bool InputManager::checkConfigModeGesture(uint32_t nowMs) {
    uint32_t held = updateHoldTimer(nowMs);
    if (held == 0 || _consumedForThisHold) return false;
    if (held >= WIFI_CONFIG_HOLD_MS) {
        _consumedForThisHold = true; // this hold is spent - checkFactoryReset() won't fire for it
        return true;
    }
    return false;
}

bool InputManager::checkFactoryReset(uint32_t nowMs) {
    uint32_t held = updateHoldTimer(nowMs);
    if (held == 0 || _consumedForThisHold) return false;
    return held >= FACTORY_RESET_HOLD_MS;
}
