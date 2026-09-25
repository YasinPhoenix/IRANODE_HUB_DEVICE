#ifndef CONFIG_MODE_INDICATOR_H
#define CONFIG_MODE_INDICATOR_H

#include <Arduino.h>
#include "Config.h"
#include "ShiftRegister.h"

// Non-blocking RED/OFF flash across every channel's indicator LED while the
// switch is in Wi-Fi configuration mode - see WifiConfigManager. Owns
// nothing persistent and never touches relay state, only the color bits:
// exiting configuration mode always happens via ESP.restart() (see
// IranodeSwitch.ino), which re-applies the real relay/color state from
// EEPROM on the next boot, so this class never needs to "restore" the real
// LED colors itself.
class ConfigModeIndicator {
public:
    // Call once, right as the device enters configuration mode, so the
    // very first flash happens on the next tick() call instead of waiting
    // up to LED_FLASH_INTERVAL_MS - the spec calls for the flash to begin
    // immediately at the 5-second gesture threshold, not on whatever
    // scheduled tick happens to follow it.
    void reset(uint32_t nowMs);

    // Call every loop() while in configuration mode. Toggles every
    // channel's indicator LED between red and off, no more often than
    // LED_FLASH_INTERVAL_MS.
    void tick(uint32_t nowMs, ShiftRegister &sr);

private:
    uint32_t _lastToggleMs = 0;
    bool _on = false;
};

#endif // CONFIG_MODE_INDICATOR_H
