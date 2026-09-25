#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>
#include "Config.h"

// Everything that survives a reboot: relay + color state (below), and the
// Wi-Fi station credentials (WifiCredentials, further down) - StateManager
// owns the former, WifiConfigManager the latter. The two live at different
// fixed EEPROM offsets (see WIFI_EEPROM_OFFSET) specifically so they don't
// share one struct: StateManager rebuilds PersistentState fresh from its
// own in-memory state on every save (see State.cpp), and if WiFi fields
// lived in that same struct they'd get silently reset to defaults on every
// relay toggle - the exact bug this project's predecessor had to work
// around by load-before-save. Two independent EEPROM.begin()-sized regions
// avoids that entirely: EEPROM.get()/put() only ever touch their own
// region of the shared RAM-backed buffer, so writing one never disturbs
// the other, and each has its own magic/version/checksum so either can be
// invalidated independently (factory reset wipes both).
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

// The network the switch should join as a station (see WifiConfigManager).
// An empty ssid (or a failed load) means "not configured yet" - the switch
// boots straight into its own configuration-mode AP instead.
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASS_LEN 64

struct WifiCredentials {
    uint8_t magic;
    uint8_t version;
    char    ssid[MAX_WIFI_SSID_LEN + 1];
    char    password[MAX_WIFI_PASS_LEN + 1];
    uint8_t checksum;
};

#define WIFI_PERSIST_MAGIC   0xC3
#define WIFI_PERSIST_VERSION 1

// Where WifiCredentials lives in the shared EEPROM buffer - right after
// PersistentState, so both regions fit in one EEPROM.begin() call.
#define WIFI_EEPROM_OFFSET (sizeof(PersistentState))

namespace Persistence {
    void begin();
    bool load(PersistentState &out);
    void save(const PersistentState &in);
    void applyDefaults(PersistentState &out);
    void invalidate();

    bool loadWifi(WifiCredentials &out);
    void saveWifi(const WifiCredentials &in);
    void invalidateWifi(); // used by factory reset, alongside invalidate()
}

#endif // PERSISTENCE_H
