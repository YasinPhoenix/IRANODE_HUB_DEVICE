#include "ConfigModeIndicator.h"

void ConfigModeIndicator::reset(uint32_t nowMs) {
    _lastToggleMs = nowMs - LED_FLASH_INTERVAL_MS; // forces tick()'s very next call to fire
    _on = false; // tick()'s first toggle flips this to true (red) immediately
}

void ConfigModeIndicator::tick(uint32_t nowMs, ShiftRegister &sr) {
    if (nowMs - _lastToggleMs < LED_FLASH_INTERVAL_MS) return;
    _lastToggleMs = nowMs;
    _on = !_on;
    for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
        sr.setColor(i, _on ? COLOR_RED : COLOR_OFF);
    }
    sr.write();
}
