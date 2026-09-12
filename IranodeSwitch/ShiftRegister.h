#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <Arduino.h>
#include "Config.h"

// Owns the 16-bit physical output word for the two 74HC595s and knows how
// to shift it out correctly. setRelay()/setColor() only touch the in-memory
// word - nothing reaches the physical pins until write() is called, so a
// caller can change several bits and still pay for exactly one shiftOut
// sequence.
class ShiftRegister {
public:
    void begin();

    void setRelay(uint8_t index, bool on);
    void setColor(uint8_t index, uint8_t color); // RGBColor 0-7

    void write();

private:
    uint16_t _state = 0;

    void setBit(uint8_t bit, bool on);
};

#endif // SHIFT_REGISTER_H
