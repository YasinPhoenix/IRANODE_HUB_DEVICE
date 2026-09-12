#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "Config.h"
#include "ShiftRegister.h"
#include "Persistence.h"

struct SwitchState {
    bool relayOn;
    uint8_t colorOn;
    uint8_t colorOff;
};

// Single source of truth for what each switch is doing right now. Owns two
// independent save timers: relay changes save on a short delay (a relay is
// a physical thing that should survive a reboot exactly as last set), color
// preference changes save on a longer delay (cosmetic, likely tweaked in a
// burst via the hub, no reason to hit flash for every intermediate pick).
class StateManager {
public:
    // p is the already-resolved (loaded-or-defaulted) persistent state -
    // this class does not do its own EEPROM resolution at boot; the main
    // .ino resolves it once, centrally, before any module touches
    // persistent state.
    void begin(const PersistentState &p, ShiftRegister &sr);

    void toggleRelay(uint8_t index, ShiftRegister &sr);
    void setColor(uint8_t index, bool forOnState, uint8_t color, ShiftRegister &sr);

    const SwitchState &get(uint8_t index) const { return _state[index]; }

    void tick(uint32_t nowMs); // call every loop() - performs any due saves

private:
    SwitchState _state[SWITCH_COUNT];
    bool _relayDirty = false;
    bool _colorDirty = false;
    uint32_t _relayChangedAtMs = 0;
    uint32_t _colorChangedAtMs = 0;

    void applyAllToShiftRegister(ShiftRegister &sr) const;
    void persistNow();
};

#endif // STATE_H
