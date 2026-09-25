#ifndef WIFI_AP_MANAGER_H
#define WIFI_AP_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"

// Packed explicitly - same reasoning as DeviceStore's DeviceRecord: keeps
// the "sum every byte except the checksum" convention safe from
// compiler-inserted padding.
#pragma pack(push, 1)
struct ApSettings {
    uint8_t magic;
    uint8_t version;
    char    ssid[AP_MAX_SSID_LEN + 1];
    char    password[AP_MAX_PASSWORD_LEN + 1];
    uint8_t maxConnections;
    uint8_t checksum;
};
#pragma pack(pop)

// The hub's own stable identifier, derived from its WiFi station MAC - used
// to build the default/unconfigured AP name "IranodeHub-<id>". Lower 32
// bits of the base MAC is plenty of uniqueness for a per-install default
// SSID and mirrors how every device elsewhere in this project is
// identified by a 32-bit hex id (see WebApi::hexId()).
inline uint32_t hubDeviceId() {
    return static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFFFFULL);
}

// Owns everything about the hub's own WiFi access point: bringing it up at
// boot (either with a saved configuration, or - if none is saved yet, or
// it's been invalidated - with the default "IranodeHub-<id>" / no-password
// AP that WebApi/WebUI treat as the unconfigured state), persisting a new
// configuration the user submits via the AP-configuration web page, and
// applying it.
//
// "Applying" a new configuration always ends in ESP.restart(): the
// simplest, most robust way to "reconfigure the AP as necessary" (every
// already-connected device has to rejoin the new network anyway, so there
// is no meaningful advantage to a live WiFi.softAP() reconfigure over a
// clean reboot into the newly-saved settings via the same code path
// begin() already uses). This is the one and only place AP-bring-up logic
// lives, so setup() and the web API both go through it instead of
// duplicating WiFi.softAP()/softAPConfig() calls.
class WifiApManager {
public:
    // Mounts LittleFS (safe to call even though DeviceStore also mounts it
    // - LittleFS.begin() on an already-mounted filesystem just returns
    // true immediately, it doesn't reformat), loads a saved configuration
    // if one exists and is valid, and brings up the AP - either with that
    // saved configuration, or with the default unconfigured one.
    void begin();

    void tick(uint32_t nowMs); // fires the delayed ESP.restart() after apply()

    bool isConfigured() const { return _configured; }

    // Never echoes the password back - the config page simply leaves the
    // password field blank rather than re-displaying a saved secret.
    const char *currentSsid() const { return _active.ssid; }
    uint8_t currentMaxConnections() const { return _active.maxConnections; }
    void defaultSsid(char *out, size_t outLen) const;

    // Validates, persists, and schedules the restart that applies it.
    // Returns false (with a human-readable reason in errorOut) without
    // touching flash if validation fails.
    bool apply(const char *ssid, const char *password, uint8_t maxConnections, String &errorOut);

private:
    ApSettings _active{};
    bool _configured = false;

    bool _restartPending = false;
    uint32_t _restartAtMs = 0;

    void applyDefault();
    void startAp();
    bool loadFromFlash(ApSettings &out) const;
    void saveToFlash(const ApSettings &in) const;
};

#endif // WIFI_AP_MANAGER_H
