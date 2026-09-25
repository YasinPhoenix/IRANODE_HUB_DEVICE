#include "Config.h"
#include "Protocol.h"
#include "ShiftRegister.h"
#include "State.h"
#include "Persistence.h"
#include "InputManager.h"
#include "HubLink.h"
#include "WifiConfigManager.h"
#include "ConfigModeIndicator.h"

ShiftRegister shiftRegister;
StateManager stateManager;
InputManager inputManager;
HubLink hubLink;
WifiConfigManager wifiConfigManager;
ConfigModeIndicator configIndicator;

void setup() {
    // 1. GPIO pin modes only - no shift-register write yet, so the relays/
    //    LEDs don't visibly pulse through an intermediate state before the
    //    real starting values are known (step 3).
    shiftRegister.begin();
    inputManager.begin();

    // 2. Resolve persistent state once, here, centrally, before anything
    //    touches the physical outputs. A corrupt or first-ever EEPROM gets
    //    safe defaults, saved once immediately.
    Persistence::begin();
    PersistentState state;
    if (!Persistence::load(state)) {
        Persistence::applyDefaults(state);
        Persistence::save(state);
    }

    // 3. Apply the resolved state and perform the ONE startup physical
    //    write.
    stateManager.begin(state, shiftRegister);

    // 4. Resolve Wi-Fi: joins the hub with saved credentials if there are
    //    any, otherwise boots straight into the device's own configuration
    //    AP - see WifiConfigManager. Either way this is non-blocking: the
    //    switch is already fully functional from step 3 onward regardless
    //    of whether a hub/network is actually in range right now.
    wifiConfigManager.begin(&hubLink, &stateManager, &shiftRegister);
    if (wifiConfigManager.inConfigMode()) {
        configIndicator.reset(millis());
    }
}

void loop() {
    uint32_t now = millis();

    uint8_t pressedIndex;
    if (inputManager.poll(pressedIndex)) {
        stateManager.toggleRelay(pressedIndex, shiftRegister);
        hubLink.reportStateChange(pressedIndex);
    }

    if (wifiConfigManager.inConfigMode()) {
        // Already provisioning - the only touch gesture armed here is
        // factory reset. checkConfigModeGesture() only matters as a
        // transition *out of* normal STA operation, so it isn't called
        // here; that also means a hold that begins in this mode is never
        // "consumed" and checkFactoryReset() fires normally at the full
        // 10 seconds - see InputManager's and WifiConfigManager's
        // comments for the full reasoning.
        if (inputManager.checkFactoryReset(now)) {
            Persistence::invalidate();
            Persistence::invalidateWifi(); // back to fully unconfigured, not just relay/color defaults
            ESP.restart();
        }
        configIndicator.tick(now, shiftRegister);
    } else {
        // Hold every touch at once for WIFI_CONFIG_HOLD_MS to drop into
        // Wi-Fi configuration mode. This fires immediately (no waiting for
        // release) and, for this same continuous hold, takes the place of
        // the 10-second factory-reset gesture rather than leading into it
        // - see InputManager::checkConfigModeGesture()'s comment.
        if (inputManager.checkConfigModeGesture(now)) {
            wifiConfigManager.enterConfigMode();
            configIndicator.reset(now); // flashing starts on this same loop() pass
        }
    }

    stateManager.tick(now);
    wifiConfigManager.tick(now); // dispatches to hubLink.tick() or the config-mode web server, per current mode
}
