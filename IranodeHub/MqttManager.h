#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "CommsManager.h"
#include "DeviceRegistry.h"
#include "DeviceStore.h"
#include "NetworkConfigManager.h"
#include "WifiApManager.h"

class MqttManager {
public:
    void begin(NetworkConfigManager *network, DeviceRegistry *registry,
               DeviceStore *store, CommsManager *comms);
    void tick(uint32_t nowMs);
    bool connected() const { return _mqtt && _mqtt->connected(); }

private:
    static const uint16_t MAX_COMMAND_BYTES = 768;
    static const uint32_t RECONNECT_INTERVAL_MS = 10000UL;
    static const uint32_t STATE_PUBLISH_INTERVAL_MS = 5000UL;

    NetworkConfigManager *_network = nullptr;
    DeviceRegistry *_registry = nullptr;
    DeviceStore *_store = nullptr;
    CommsManager *_comms = nullptr;
    WiFiClient _plainClient;
    WiFiClientSecure _tlsClient;
    MqttClient *_mqtt = nullptr;
    String _topicPrefix;
    String _availabilityTopic;
    String _stateTopic;
    String _commandTopic;
    String _ackTopic;
    uint32_t _lastConnectAttemptMs = 0;
    uint32_t _lastStatePublishMs = 0;
    bool _hasConnectAttempted = false;

    void connect(uint32_t nowMs);
    void disconnect();
    bool publish(const String &topic, const String &payload, bool retained, uint8_t qos);
    void publishAvailability(bool online);
    void publishState(uint32_t nowMs);
    void processCommand(const char *payload, size_t length, bool retained);
    void publishAck(const char *requestId, const char *command, bool ok, const char *status);
    bool parseDeviceId(const char *text, uint32_t &id) const;
};

#endif
