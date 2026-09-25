#include "WifiConfigManager.h"
#include "ConfigUI.h"
#include <string.h>

// This file is the one exception to the project's "no Arduino String"
// rule: ESP8266WebServer's own request-argument API (_server.arg()) only
// ever returns String, there is no char*-returning alternative, so
// handleSave() below can't avoid touching it entirely. It's scoped as
// tightly as the library allows - two function-local temporaries, used
// only for length validation and an immediate toCharArray() copy into the
// fixed-size WifiCredentials buffers, then out of scope - and it only
// runs at all while the device is in the rare, temporary configuration
// mode, never in the steady-state hot loop the "no String" rule is really
// protecting.
void WifiConfigManager::begin(HubLink *hubLink, StateManager *state, ShiftRegister *sr) {
    _hubLink = hubLink;

    WifiCredentials creds;
    bool haveCreds = Persistence::loadWifi(creds) && creds.ssid[0] != '\0';

    if (haveCreds) {
        _mode = STA_NORMAL;
        WiFi.mode(WIFI_STA);
        _hubLink->begin(state, sr, creds.ssid, creds.password);
    } else {
        enterConfigMode(); // first boot / wiped credentials - straight to setup
    }
}

void WifiConfigManager::enterConfigMode() {
    if (_mode == AP_CONFIG) return; // already there - nothing to do

    _mode = AP_CONFIG;

    WiFi.disconnect(true); // drop any STA association before switching mode
    WiFi.mode(WIFI_AP);

    IPAddress ip, subnet;
    ip.fromString(CONFIG_AP_IP_STR);
    subnet.fromString(CONFIG_AP_SUBNET_STR);
    WiFi.softAPConfig(ip, ip, subnet);

    snprintf(_apSsid, sizeof(_apSsid), "%s%08x", CONFIG_AP_SSID_PREFIX, ESP.getChipId());
    WiFi.softAP(_apSsid, CONFIG_AP_PASSWORD);

    if (!_serverStarted) {
        _server.on("/", HTTP_GET, [this]() { handleRoot(); });
        _server.on("/api/wifi", HTTP_GET, [this]() { handleGetStatus(); });
        _server.on("/api/wifi", HTTP_POST, [this]() { handleSave(); });
        _server.begin();
        _serverStarted = true;
    }
}

void WifiConfigManager::tick(uint32_t nowMs) {
    if (_mode == AP_CONFIG) {
        _server.handleClient();
        if (_restartPending && nowMs >= _restartAtMs) {
            ESP.restart();
        }
        return;
    }
    _hubLink->tick(nowMs);
}

void WifiConfigManager::handleRoot() {
    _server.send_P(200, "text/html", CONFIG_HTML);
}

void WifiConfigManager::handleGetStatus() {
    char json[48];
    snprintf(json, sizeof(json), "{\"apSsid\":\"%s\"}", _apSsid);
    _server.send(200, "application/json", json);
}

void WifiConfigManager::handleSave() {
    String ssidArg = _server.arg("ssid");
    String passArg = _server.arg("password");

    if (ssidArg.length() == 0 || ssidArg.length() > MAX_WIFI_SSID_LEN) {
        _server.send(400, "text/plain", "ssid must be 1-32 characters");
        return;
    }
    if (passArg.length() != 0 && (passArg.length() < 8 || passArg.length() > MAX_WIFI_PASS_LEN)) {
        _server.send(400, "text/plain", "password must be empty or 8-64 characters");
        return;
    }

    WifiCredentials creds;
    memset(&creds, 0, sizeof(creds));
    ssidArg.toCharArray(creds.ssid, sizeof(creds.ssid));
    passArg.toCharArray(creds.password, sizeof(creds.password));
    Persistence::saveWifi(creds);

    _server.send(200, "text/plain", "saved");
    scheduleRestart();
}

void WifiConfigManager::scheduleRestart() {
    _restartPending = true;
    _restartAtMs = millis() + WIFI_CONFIG_RESTART_DELAY_MS;
}
