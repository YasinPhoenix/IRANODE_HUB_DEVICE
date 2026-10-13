#ifndef NETWORK_CONFIG_MANAGER_H
#define NETWORK_CONFIG_MANAGER_H

#include <Arduino.h>
#include "Config.h"

#define UPLINK_SSID_LEN       33
#define UPLINK_PASSWORD_LEN   64
#define MQTT_HOST_LEN         96
#define MQTT_USERNAME_LEN     65
#define MQTT_PASSWORD_LEN     97
#define MQTT_CA_CERT_LEN      2049

#define NETWORK_CONFIG_PATH   "/config/network.bin"
#define NETWORK_CONFIG_DELAY_MS 1200UL

#pragma pack(push, 1)
struct NetworkSettings {
    uint8_t magic;
    uint8_t version;
    char uplinkSsid[UPLINK_SSID_LEN];
    char uplinkPassword[UPLINK_PASSWORD_LEN];
    char mqttHost[MQTT_HOST_LEN];
    uint16_t mqttPort;
    char mqttUsername[MQTT_USERNAME_LEN];
    char mqttPassword[MQTT_PASSWORD_LEN];
    uint8_t mqttTls;
    char mqttCaCert[MQTT_CA_CERT_LEN];
    uint8_t checksum;
};
#pragma pack(pop)

class NetworkConfigManager {
public:
    void begin();
    void tick(uint32_t nowMs);

    const NetworkSettings &settings() const { return _settings; }
    bool mqttConfigured() const { return _settings.mqttHost[0] != '\0'; }
    bool uplinkConfigured() const { return _settings.uplinkSsid[0] != '\0'; }
    bool apply(const NetworkSettings &settings, String &errorOut);

private:
    NetworkSettings _settings{};
    bool _restartPending = false;
    uint32_t _restartAtMs = 0;
    uint32_t _lastWifiAttemptMs = 0;
    bool _wifiAttempted = false;
    bool _timeSyncStarted = false;
    bool _wifiWasConnected = false;

    bool loadFromFlash(NetworkSettings &out) const;
    bool saveToFlash(const NetworkSettings &settings) const;
    void startUplink();
};

#endif
