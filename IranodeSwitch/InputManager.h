#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "Config.h"

// Simple polling debounce: touch pins are active-LOW (INPUT_PULLUP). A press
// is only reported on the falling edge, and only once TOUCH_DEBOUNCE_MS has
// passed since the last press was reported for that switch. No event bus,
// no interrupts - poll() is meant to be called once per loop() iteration.
//
// checkConfigModeGesture()/checkFactoryReset() both watch the same
// all-touch hold and share one underlying timer - see their comments for
// how the two stay non-conflicting despite that.
class InputManager {
public:
    void begin();

    // Returns true (with outIndex set) if a switch was newly pressed since
    // the last call. Any second/third simultaneous press is caught on the
    // next loop() iteration, a fraction of a millisecond later.
    bool poll(uint8_t &outIndex);

    // True exactly once per continuous all-touch hold, on the loop() call
    // where the hold first reaches WIFI_CONFIG_HOLD_MS. Marks the current
    // hold "consumed" so checkFactoryReset() can no longer fire for it -
    // see that method's comment for why, and IranodeSwitch.ino for which
    // of the two gets called in which mode. Individual relays may toggle
    // once as fingers land (poll() still fires on those press edges) -
    // harmless, same as it always was for the factory-reset gesture.
    //
    // Only meaningful (and only ever called by IranodeSwitch.ino) while
    // the switch is in normal STA operation - entering configuration mode
    // a second time while already in it is a no-op there.
    bool checkConfigModeGesture(uint32_t nowMs);

    // True exactly once per continuous all-touch hold, on the loop() call
    // where the hold first reaches FACTORY_RESET_HOLD_MS - but only for a
    // hold that never triggered checkConfigModeGesture(). In practice that
    // means: a hold started while the device is already in configuration
    // mode (either because it booted straight there with no saved Wi-Fi
    // credentials, or because an earlier, separate hold's 5-second mark
    // already moved it there) reaches 10s and fires here normally; a hold
    // started in normal STA operation gets diverted into
    // checkConfigModeGesture() at the 5-second mark instead and can never
    // also fire this for the same hold, however long it's kept down - the
    // two gestures are only ever alternatives, never a chain, for any
    // single continuous hold. Only ever called by IranodeSwitch.ino while
    // already in configuration mode - see that comment there.
    bool checkFactoryReset(uint32_t nowMs);

private:
    bool _lastState[SWITCH_COUNT];
    uint32_t _lastFireMs[SWITCH_COUNT];

    uint32_t _allHeldSinceMs = 0;      // 0 = not currently all-held
    bool _consumedForThisHold = false; // true once checkConfigModeGesture() has fired for the current hold

    bool allCurrentlyHeld() const;

    // Shared by both gesture checks: updates the continuous all-held
    // timer and returns how long (ms) it's been held, or 0 if not
    // currently all-held (which also clears state for the next hold).
    uint32_t updateHoldTimer(uint32_t nowMs);
};

#endif // INPUT_MANAGER_H
