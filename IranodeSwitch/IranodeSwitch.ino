#include "Config.h"
#include "Protocol.h"
#include "ShiftRegister.h"
#include "State.h"
#include "Persistence.h"
#include "InputManager.h"
#include "HubLink.h"

ShiftRegister shiftRegister;
StateManager stateManager;
InputManager inputManager;
HubLink hubLink;

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

    // 4. Kick off the hub connection. Non-blocking: returns immediately
    //    whether or not a hub is actually in range right now - the switch
    //    is already fully functional from step 3 onward regardless.
    hubLink.begin(&stateManager, &shiftRegister);
}

void loop() {
    uint32_t now = millis();

    uint8_t pressedIndex;
    if (inputManager.poll(pressedIndex)) {
        stateManager.toggleRelay(pressedIndex, shiftRegister);
        hubLink.reportStateChange(pressedIndex);
    }

    // Hold every touch at once for FACTORY_RESET_HOLD_MS to reset relay/
    // color preferences to factory defaults and reboot. There's no local
    // WiFi config to forget in this build - the hub connection is fixed at
    // compile time - so this is purely a "get back to a known state" reset.
    if (inputManager.checkFactoryReset(now)) {
        Persistence::invalidate();
        ESP.restart();
    }

    stateManager.tick(now);
    hubLink.tick(now);
}
