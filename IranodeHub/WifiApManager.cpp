#include "WifiApManager.h"
#include <LittleFS.h>
#include <string.h>

static uint8_t computeChecksum(const ApSettings &s) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&s);
    uint16_t sum = 0;
    for (size_t i = 0; i < sizeof(ApSettings) - 1; i++) {
        sum += bytes[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

#define AP_SETTINGS_MAGIC   0x41 // 'A'
#define AP_SETTINGS_VERSION 1

void WifiApManager::defaultSsid(char *out, size_t outLen) const {
    snprintf(out, outLen, "%s%08x", AP_SSID_PREFIX, hubDeviceId());
}

bool WifiApManager::loadFromFlash(ApSettings &out) const {
    File f = LittleFS.open(AP_CONFIG_SAVE_PATH, "r");
    if (!f) return false;

    int got = f.read(reinterpret_cast<uint8_t *>(&out), sizeof(out));
    f.close();

    if (got != (int)sizeof(out)) return false;
    if (out.magic != AP_SETTINGS_MAGIC) return false;
    if (out.version != AP_SETTINGS_VERSION) return false;
    if (computeChecksum(out) != out.checksum) return false;
    if (out.ssid[0] == '\0') return false; // an empty SSID is never a valid saved config
    return true;
}

void WifiApManager::saveToFlash(const ApSettings &in) const {
    ApSettings copy = in;
    copy.magic = AP_SETTINGS_MAGIC;
    copy.version = AP_SETTINGS_VERSION;
    copy.checksum = computeChecksum(copy);

    if (!LittleFS.exists("/config")) {
        LittleFS.mkdir("/config");
    }
    File f = LittleFS.open(AP_CONFIG_SAVE_PATH, "w");
    if (!f) {
        Serial.println("[WifiApManager] flash write failed");
        return;
    }
    f.write(reinterpret_cast<const uint8_t *>(&copy), sizeof(copy));
    f.close();
}

void WifiApManager::applyDefault() {
    memset(&_active, 0, sizeof(_active));
    defaultSsid(_active.ssid, sizeof(_active.ssid));
    _active.password[0] = '\0'; // open network, per the unconfigured-AP spec
    _active.maxConnections = AP_DEFAULT_MAX_CONN;
    _configured = false;
}

void WifiApManager::startAp() {
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    IPAddress hubIp, subnet;
    hubIp.fromString(HUB_IP_STR);
    subnet.fromString(SUBNET_STR);
    WiFi.softAPConfig(hubIp, hubIp, subnet);
    WiFi.softAP(_active.ssid, _active.password, 1, false, _active.maxConnections);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
}

void WifiApManager::begin() {
    // true = format on first-ever boot / failed mount. Idempotent with
    // DeviceStore's own LittleFS.begin(true) call later in setup() - this
    // one just needs to run first, since the AP has to be up before
    // anything else.
    if (!LittleFS.begin(true)) {
        Serial.println("[WifiApManager] LittleFS mount failed");
    }

    ApSettings saved;
    if (loadFromFlash(saved)) {
        _active = saved;
        _configured = true;
    } else {
        applyDefault();
    }

    startAp();
}

bool WifiApManager::apply(const char *ssid, const char *password, uint8_t maxConnections, String &errorOut) {
    size_t ssidLen = strlen(ssid);
    size_t passLen = strlen(password);

    if (ssidLen == 0 || ssidLen > AP_MAX_SSID_LEN) {
        errorOut = "SSID must be 1-" + String(AP_MAX_SSID_LEN) + " characters";
        return false;
    }
    if (passLen != 0 && (passLen < AP_MIN_PASSWORD_LEN || passLen > AP_MAX_PASSWORD_LEN)) {
        errorOut = "Password must be empty or " + String(AP_MIN_PASSWORD_LEN) + "-" + String(AP_MAX_PASSWORD_LEN) + " characters";
        return false;
    }
    if (maxConnections < AP_MIN_MAX_CONN || maxConnections > AP_MAX_MAX_CONN) {
        errorOut = "Max connections must be " + String(AP_MIN_MAX_CONN) + "-" + String(AP_MAX_MAX_CONN);
        return false;
    }

    ApSettings next{};
    strncpy(next.ssid, ssid, sizeof(next.ssid) - 1);
    strncpy(next.password, password, sizeof(next.password) - 1);
    next.maxConnections = maxConnections;
    saveToFlash(next);

    _restartPending = true;
    _restartAtMs = millis() + AP_RESTART_DELAY_MS;
    return true;
}

void WifiApManager::tick(uint32_t nowMs) {
    if (_restartPending && nowMs >= _restartAtMs) {
        ESP.restart();
    }
}
