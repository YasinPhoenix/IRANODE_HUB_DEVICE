#include "State.h"

void StateManager::begin(const PersistentState &p, ShiftRegister &sr) {
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        _state[i].relayOn  = (p.relayStates >> i) & 1;
        _state[i].colorOn  = p.colorOn[i];
        _state[i].colorOff = p.colorOff[i];
    }
    applyAllToShiftRegister(sr);
    sr.write(); // the one and only startup physical write
}

void StateManager::applyAllToShiftRegister(ShiftRegister &sr) const {
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        sr.setRelay(i, _state[i].relayOn);
        sr.setColor(i, _state[i].relayOn ? _state[i].colorOn : _state[i].colorOff);
    }
}

void StateManager::toggleRelay(uint8_t index, ShiftRegister &sr) {
    if (index >= SWITCH_COUNT) return;
    _state[index].relayOn = !_state[index].relayOn;
    sr.setRelay(index, _state[index].relayOn);
    sr.setColor(index, _state[index].relayOn ? _state[index].colorOn : _state[index].colorOff);
    sr.write();
    _relayDirty = true;
    _relayChangedAtMs = millis();
}

void StateManager::setColor(uint8_t index, bool forOnState, uint8_t color, ShiftRegister &sr) {
    if (index >= SWITCH_COUNT || color >= COLOR_COUNT) return;
    if (forOnState) _state[index].colorOn = color;
    else            _state[index].colorOff = color;

    // Only touch the physical LED immediately if that's the color currently
    // showing (relay state matches the half of the pair just changed).
    if (_state[index].relayOn == forOnState) {
        sr.setColor(index, color);
        sr.write();
    }
    _colorDirty = true;
    _colorChangedAtMs = millis();
}

void StateManager::tick(uint32_t nowMs) {
    if (_relayDirty && (nowMs - _relayChangedAtMs >= RELAY_SAVE_DELAY_MS)) {
        persistNow();
        _relayDirty = false;
    }
    if (_colorDirty && (nowMs - _colorChangedAtMs >= COLOR_SAVE_DELAY_MS)) {
        persistNow();
        _colorDirty = false;
    }
}

void StateManager::persistNow() {
    // No load-before-save needed here: unlike the old build, this struct
    // has no fields owned by another module (no AP credentials sharing the
    // block), so StateManager is free to construct it fresh from its own
    // in-memory _state[] every time.
    PersistentState p;
    Persistence::applyDefaults(p); // gives magic/version + safe fallback colors, all overwritten below
    p.relayStates = 0;
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        if (_state[i].relayOn) p.relayStates |= (1 << i);
        p.colorOn[i]  = _state[i].colorOn;
        p.colorOff[i] = _state[i].colorOff;
    }
    Persistence::save(p);
}
