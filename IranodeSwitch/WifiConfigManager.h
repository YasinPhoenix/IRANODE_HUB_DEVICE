#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "Config.h"
#include "Persistence.h"
#include "HubLink.h"

// Owns the switch's two WiFi operating modes and the transition between
// them - the switch-side mirror of the hub's WifiApManager.
//
//   STA_NORMAL: joins the configured network and talks to the hub - all of
//   that is still HubLink's job, unchanged; WifiConfigManager just hands it
//   the saved credentials instead of the old compile-time constants.
//
//   AP_CONFIG: hosts its own open, temporary setup AP ("IranodeSwitch-
//   <id>") with a small web page for entering the real WiFi credentials.
//   Entered either at boot (no valid saved credentials) or live, from
//   normal operation, via the 5-second all-touch gesture (see
//   IranodeSwitch.ino / InputManager) - see enterConfigMode().
//
// Saving new credentials in AP_CONFIG mode always ends in ESP.restart():
// the simplest possible way to "reconfigure as necessary" (the same choice
// the hub's WifiApManager makes), and it means this class never has to
// implement a live AP-mode -> STA-mode teardown/bring-up path - the normal
// boot sequence already does that correctly.
class WifiConfigManager {
public:
    // ssid/password aren't known yet at this point - state/sr are just
    // forwarded straight through to HubLink::begin() once (if) valid
    // saved credentials are found. See HubLink::begin()'s signature.
    void begin(HubLink *hubLink, StateManager *state, ShiftRegister *sr);
    void tick(uint32_t nowMs);

    // Called from IranodeSwitch.ino the moment the 5-second gesture fires.
    // Non-blocking - WiFi.mode()/softAP() return quickly, nothing here
    // waits on the network. A no-op if already in configuration mode.
    void enterConfigMode();

    bool inConfigMode() const { return _mode == AP_CONFIG; }

private:
    enum Mode { STA_NORMAL, AP_CONFIG };
    Mode _mode = STA_NORMAL;

    HubLink *_hubLink = nullptr;
    ESP8266WebServer _server{80};
    bool _serverStarted = false;

    bool _restartPending = false;
    uint32_t _restartAtMs = 0;

    char _apSsid[24]; // "IranodeSwitch-" (14) + 8 hex digits + '\0'

    void handleRoot();
    void handleGetStatus();
    void handleSave();
    void scheduleRestart();
};

#endif // WIFI_CONFIG_MANAGER_H
