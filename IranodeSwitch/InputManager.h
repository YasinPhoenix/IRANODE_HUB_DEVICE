#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "Config.h"

// Simple polling debounce: touch pins are active-LOW (INPUT_PULLUP). A press
// is only reported on the falling edge, and only once TOUCH_DEBOUNCE_MS has
// passed since the last press was reported for that switch. No event bus,
// no interrupts - poll() is meant to be called once per loop() iteration.
class InputManager {
public:
    void begin();

    // Returns true (with outIndex set) if a switch was newly pressed since
    // the last call. Any second/third simultaneous press is caught on the
    // next loop() iteration, a fraction of a millisecond later.
    bool poll(uint8_t &outIndex);

    // Returns true once every touch has been held down continuously for
    // FACTORY_RESET_HOLD_MS. Independent of poll()'s per-switch edge state -
    // both simply read the same raw pins. Individual relays may toggle once
    // as fingers land (poll() still fires on those press edges) - harmless,
    // since a factory reset wipes and reboots to defaults right after.
    bool checkFactoryReset(uint32_t nowMs);

private:
    bool _lastState[SWITCH_COUNT];
    uint32_t _lastFireMs[SWITCH_COUNT];
    uint32_t _allHeldSinceMs = 0; // 0 = not currently all-held
};

#endif // INPUT_MANAGER_H
