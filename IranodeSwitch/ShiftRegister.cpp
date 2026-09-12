#include "ShiftRegister.h"

void ShiftRegister::begin() {
    pinMode(SR_DATA_PIN, OUTPUT);
    pinMode(SR_CLOCK_PIN, OUTPUT);
    pinMode(SR_LATCH_PIN, OUTPUT);
    // Deliberately no shiftOut() here. The first physical write happens once
    // the real starting state (from validated EEPROM, or safe defaults) is
    // known - see main .ino boot sequence - so the relays never visibly
    // pulse through an intermediate "all off" state during boot.
}

void ShiftRegister::setRelay(uint8_t index, bool on) {
    if (index >= SWITCH_COUNT) return;
    setBit(RELAY_BITS[index], on);
}

void ShiftRegister::setColor(uint8_t index, uint8_t color) {
    if (index >= SWITCH_COUNT || color >= COLOR_COUNT) return;
    uint8_t bits = COLOR_RGB_BITS[color];
    setBit(RED_BITS[index],   bits & 0b001);
    setBit(GREEN_BITS[index], bits & 0b010);
    setBit(BLUE_BITS[index],  bits & 0b100);
}

void ShiftRegister::write() {
    digitalWrite(SR_LATCH_PIN, LOW);
    // High byte (bits 8-15) goes out first, low byte (bits 0-7) second -
    // matches the physical transmission order confirmed in the legacy
    // ShiftRegisterManager.h. Both are always fully re-sent, so the chips'
    // internal storage never holds a stale mix of old and new bits.
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, (_state >> 8) & 0xFF);
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, _state & 0xFF);
    digitalWrite(SR_LATCH_PIN, HIGH);
}

void ShiftRegister::setBit(uint8_t bit, bool on) {
    if (on) _state |= (1u << bit);
    else    _state &= ~(1u << bit);
}
