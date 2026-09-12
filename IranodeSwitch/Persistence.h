#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>
#include "Config.h"

// Everything that survives a reboot. No AP credentials in this build - the
// hub connection is fixed at compile time (see Config.h) - so this struct
// is just relay + color state, and StateManager is its only owner.
//
// colorOn/colorOff are always sized for 4 switches (not SWITCH_COUNT) so the
// EEPROM layout stays stable even if a unit is later reflashed with a
// different SWITCH_COUNT.
struct PersistentState {
    uint8_t magic;
    uint8_t version;
    uint8_t relayStates;
    uint8_t colorOn[4];
    uint8_t colorOff[4];
    uint8_t checksum;
};

#define PERSIST_MAGIC   0xA5
#define PERSIST_VERSION 1

namespace Persistence {
    void begin();
    bool load(PersistentState &out);
    void save(const PersistentState &in);
    void applyDefaults(PersistentState &out);
    void invalidate();
}

#endif // PERSISTENCE_H
