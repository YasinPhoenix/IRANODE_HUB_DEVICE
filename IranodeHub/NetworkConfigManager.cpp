#include "NetworkConfigManager.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

static const uint8_t NETWORK_CONFIG_MAGIC = 0x4E;
static const uint8_t NETWORK_CONFIG_VERSION = 1;

static uint8_t computeChecksum(const NetworkSettings &settings) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&settings);
    uint16_t sum = 0;
    for (size_t i = 0; i < sizeof(NetworkSettings) - 1; i++) sum += bytes[i];
    return static_cast<uint8_t>(sum & 0xFF);
}

static bool fits(const char *value, size_t capacity) {
    return value && strlen(value) < capacity;
}

bool NetworkConfigManager::loadFromFlash(NetworkSettings &out) const {
    File file = LittleFS.open(NETWORK_CONFIG_PATH, "r");
    if (!file) return false;
    int read = file.read(reinterpret_cast<uint8_t *>(&out), sizeof(out));
    file.close();

    return read == static_cast<int>(sizeof(out)) &&
           out.magic == NETWORK_CONFIG_MAGIC &&
           out.version == NETWORK_CONFIG_VERSION &&
           computeChecksum(out) == out.checksum &&
           out.uplinkSsid[UPLINK_SSID_LEN - 1] == '\0' &&
           out.uplinkPassword[UPLINK_PASSWORD_LEN - 1] == '\0' &&
           out.mqttHost[MQTT_HOST_LEN - 1] == '\0' &&
           out.mqttUsername[MQTT_USERNAME_LEN - 1] == '\0' &&
           out.mqttPassword[MQTT_PASSWORD_LEN - 1] == '\0' &&
           out.mqttCaCert[MQTT_CA_CERT_LEN - 1] == '\0' &&
           out.mqttTls <= 1;
}

bool NetworkConfigManager::saveToFlash(const NetworkSettings &settings) const {
    NetworkSettings copy = settings;
    copy.magic = NETWORK_CONFIG_MAGIC;
    copy.version = NETWORK_CONFIG_VERSION;
    copy.checksum = computeChecksum(copy);

    if (!LittleFS.exists("/config") && !LittleFS.mkdir("/config")) return false;
    File file = LittleFS.open(NETWORK_CONFIG_PATH, "w");
    if (!file) return false;
    size_t written = file.write(reinterpret_cast<const uint8_t *>(&copy), sizeof(copy));
    file.close();
    return written == sizeof(copy);
}

void NetworkConfigManager::startUplink() {
    if (!_settings.uplinkSsid[0]) return;
    WiFi.mode(WIFI_AP_STA);
    Serial.println("[NetworkConfigManager] connecting to router Wi-Fi");
    WiFi.begin(_settings.uplinkSsid, _settings.uplinkPassword);
    _wifiAttempted = true;
    _lastWifiAttemptMs = millis();
}

void NetworkConfigManager::begin() {
    NetworkSettings saved{};
    if (loadFromFlash(saved)) _settings = saved;
    else if (LittleFS.exists(NETWORK_CONFIG_PATH)) {
        Serial.println("[NetworkConfigManager] saved settings invalid; using defaults");
    }
    startUplink();
}

void NetworkConfigManager::tick(uint32_t nowMs) {
    if (_restartPending && static_cast<int32_t>(nowMs - _restartAtMs) >= 0) {
        ESP.restart();
        return;
    }

    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    if (wifiConnected && !_wifiWasConnected) {
        Serial.println("[NetworkConfigManager] router Wi-Fi connected");
    } else if (!wifiConnected && _wifiWasConnected) {
        Serial.println("[NetworkConfigManager] router Wi-Fi disconnected");
    }
    _wifiWasConnected = wifiConnected;

    if (_settings.uplinkSsid[0] && !wifiConnected &&
        (!_wifiAttempted || nowMs - _lastWifiAttemptMs >= 15000UL)) {
        Serial.println("[NetworkConfigManager] retrying router Wi-Fi connection");
        WiFi.begin(_settings.uplinkSsid, _settings.uplinkPassword);
        _wifiAttempted = true;
        _lastWifiAttemptMs = nowMs;
    }

    if (wifiConnected && !_timeSyncStarted) {
        configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
        _timeSyncStarted = true;
    } else if (!wifiConnected) {
        _timeSyncStarted = false;
    }
}

bool NetworkConfigManager::apply(const NetworkSettings &settings, String &errorOut) {
    if (!fits(settings.uplinkSsid, sizeof(settings.uplinkSsid)) ||
        !fits(settings.uplinkPassword, sizeof(settings.uplinkPassword)) ||
        !fits(settings.mqttHost, sizeof(settings.mqttHost)) ||
        !fits(settings.mqttUsername, sizeof(settings.mqttUsername)) ||
        !fits(settings.mqttPassword, sizeof(settings.mqttPassword)) ||
        !fits(settings.mqttCaCert, sizeof(settings.mqttCaCert))) {
        errorOut = "one or more fields are too long";
        return false;
    }

    if (settings.uplinkSsid[0] && strlen(settings.uplinkSsid) > 32) {
        errorOut = "router SSID must be at most 32 bytes";
        return false;
    }
    if (settings.mqttHost[0] && settings.mqttPort == 0) {
        errorOut = "MQTT port must be between 1 and 65535";
        return false;
    }
    if (settings.mqttHost[0] && settings.mqttTls && !settings.mqttCaCert[0]) {
        errorOut = "TLS requires a CA certificate";
        return false;
    }
    if (settings.mqttTls > 1) {
        errorOut = "invalid TLS setting";
        return false;
    }
    if (!saveToFlash(settings)) {
        errorOut = "failed to save network settings";
        return false;
    }

    _settings = settings;
    _restartPending = true;
    _restartAtMs = millis() + NETWORK_CONFIG_DELAY_MS;
    return true;
}
