#include "MqttManager.h"
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static String formatId(uint32_t id) {
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%08x", id);
    return String(buffer);
}

static bool channelExists(DeviceStore *store, uint32_t deviceId, int channel) {
    if (channel < 1 || channel > MAX_CHANNELS) return false;
    DeviceRecord record;
    return store->getDetail(deviceId, record) && channel <= record.channelCount;
}

void MqttManager::begin(NetworkConfigManager *network, DeviceRegistry *registry,
                        DeviceStore *store, CommsManager *comms) {
    _network = network;
    _registry = registry;
    _store = store;
    _comms = comms;

    _topicPrefix = "iranode/v1/" + formatId(hubDeviceId());
    _availabilityTopic = _topicPrefix + "/availability";
    _stateTopic = _topicPrefix + "/state";
    _commandTopic = _topicPrefix + "/command";
    _ackTopic = _topicPrefix + "/ack";
}

void MqttManager::disconnect() {
    if (_mqtt) {
        if (_mqtt->connected()) publishAvailability(false);
        _mqtt->stop();
        delete _mqtt;
        _mqtt = nullptr;
    }
}

void MqttManager::connect(uint32_t nowMs) {
    _hasConnectAttempted = true;
    _lastConnectAttemptMs = nowMs;

    const NetworkSettings &settings = _network->settings();
    if (!settings.mqttHost[0] || WiFi.status() != WL_CONNECTED) return;
    if (settings.mqttTls && time(nullptr) < 1700000000) {
        Serial.println("[MQTT] waiting for network time before TLS verification");
        return;
    }

    disconnect();
    if (settings.mqttTls) {
        _tlsClient.setCACert(settings.mqttCaCert);
        _tlsClient.setHandshakeTimeout(10);
        _mqtt = new MqttClient(static_cast<Client &>(_tlsClient));
    } else {
        _mqtt = new MqttClient(static_cast<Client &>(_plainClient));
    }

    String clientId = "iranode-" + formatId(hubDeviceId());
    _mqtt->setId(clientId);
    _mqtt->setKeepAliveInterval(30);
    _mqtt->setConnectionTimeout(5000);
    _mqtt->setCleanSession(true);
    if (settings.mqttUsername[0]) {
        _mqtt->setUsernamePassword(settings.mqttUsername, settings.mqttPassword);
    }

    _mqtt->beginWill(_availabilityTopic, true, 1);
    _mqtt->print("{\"online\":false}");
    _mqtt->endWill();

    if (!_mqtt->connect(settings.mqttHost, settings.mqttPort)) {
        Serial.print("[MQTT] connect failed, code ");
        Serial.println(_mqtt->connectError());
        disconnect();
        return;
    }

    Serial.println("[MQTT] connected");
    if (_mqtt->subscribe(_commandTopic, 1) <= 0) {
        Serial.println("[MQTT] command subscription failed");
    }
    publishAvailability(true);
    publishState(nowMs);
    _lastStatePublishMs = nowMs;
}

bool MqttManager::publish(const String &topic, const String &payload, bool retained, uint8_t qos) {
    if (!_mqtt || !_mqtt->connected()) return false;
    if (_mqtt->beginMessage(topic, payload.length(), retained, qos) == 0) return false;
    size_t written = _mqtt->write(
        reinterpret_cast<const uint8_t *>(payload.c_str()), payload.length());
    int result = _mqtt->endMessage();
    return written == payload.length() && result == 1;
}

void MqttManager::publishAvailability(bool online) {
    String payload = online ? "{\"online\":true}" : "{\"online\":false}";
    if (!publish(_availabilityTopic, payload, true, 1)) {
        Serial.println("[MQTT] availability publish failed");
    }
}

void MqttManager::publishState(uint32_t nowMs) {
    if (!_mqtt || !_mqtt->connected()) return;

    JsonDocument doc;
    JsonArray devices = doc.to<JsonArray>();
    uint8_t indices[MAX_KNOWN_DEVICES];
    uint8_t count = _registry->sortedIndices(indices, MAX_KNOWN_DEVICES);

    for (uint8_t i = 0; i < count; i++) {
        const KnownDevice &device = _registry->at(indices[i]);
        JsonObject item = devices.add<JsonObject>();
        item["id"] = formatId(device.deviceId);
        item["online"] = device.online;
        if (device.lastSeenMs > 0) {
            item["lastSeenSec"] = (nowMs - device.lastSeenMs) / 1000;
        }

        DeviceRecord record;
        if (!_store->getDetail(device.deviceId, record)) continue;
        item["name"] = record.name;
        item["type"] = record.deviceType;
        char fw[8];
        snprintf(fw, sizeof(fw), "%u.%u", record.fwMajor, record.fwMinor);
        item["fw"] = fw;
        item["channels"] = record.channelCount;

        if (record.deviceType == DEVICE_TYPE_WALL_SWITCH) {
            const WallSwitchTypeData *sw =
                reinterpret_cast<const WallSwitchTypeData *>(record.typeData);
            JsonObject switchData = item["switch"].to<JsonObject>();
            JsonArray channels = switchData["channels"].to<JsonArray>();
            for (uint8_t ch = 0; ch < record.channelCount && ch < MAX_CHANNELS; ch++) {
                JsonObject channel = channels.add<JsonObject>();
                channel["relay"] = (sw->relayStates & (1 << ch)) != 0;
                channel["colorOn"] = sw->colorOn[ch];
                channel["colorOff"] = sw->colorOff[ch];
                channel["name"] = record.channelNames[ch];
            }
        }
    }

    String payload;
    serializeJson(doc, payload);
    if (!publish(_stateTopic, payload, true, 1)) {
        Serial.println("[MQTT] state publish failed");
    }
}

bool MqttManager::parseDeviceId(const char *text, uint32_t &id) const {
    if (!text || strlen(text) != 8) return false;
    id = 0;
    for (uint8_t i = 0; i < 8; i++) {
        char c = text[i];
        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return false;
        id = (id << 4) | digit;
    }
    return true;
}

void MqttManager::publishAck(const char *requestId, const char *command,
                             bool ok, const char *status) {
    JsonDocument doc;
    doc["requestId"] = requestId;
    doc["command"] = command;
    doc["ok"] = ok;
    doc["status"] = status;
    String payload;
    serializeJson(doc, payload);
    if (!publish(_ackTopic, payload, false, 1)) {
        Serial.println("[MQTT] command acknowledgement publish failed");
    }
}

void MqttManager::processCommand(const char *payload, size_t length, bool retained) {
    JsonDocument doc;
    DeserializationError parseError = deserializeJson(doc, payload, length);
    if (parseError) {
        Serial.print("[MQTT] invalid command JSON: ");
        Serial.println(parseError.c_str());
        return;
    }

    const char *requestId = doc["requestId"] | "";
    const char *command = doc["command"] | "";
    const char *deviceText = doc["id"] | "";
    if (!requestId[0] || !command[0]) {
        Serial.println("[MQTT] command missing requestId or command");
        return;
    }
    if (retained) {
        publishAck(requestId, command, false, "retained_command_rejected");
        return;
    }

    uint32_t deviceId;
    bool hasDeviceId = parseDeviceId(deviceText, deviceId);
    if (!hasDeviceId) {
        publishAck(requestId, command, false, "invalid_device_id");
        return;
    }
    KnownDevice device;
    if (!_registry->find(deviceId, device)) {
        publishAck(requestId, command, false, "unknown_device");
        return;
    }

    bool ok = false;
    const char *status = "invalid_command";
    uint8_t channel = 0;

    if (strcmp(command, "relay") == 0) {
        JsonVariantConst value = doc["value"];
        bool desired;
        bool valueValid = false;
        if (value.is<bool>()) {
            desired = value.as<bool>();
            valueValid = true;
        } else if (value.is<int>() && (value.as<int>() == 0 || value.as<int>() == 1)) {
            desired = value.as<int>() == 1;
            valueValid = true;
        }
        int requestedChannel = doc["channel"] | 0;
        if (valueValid && channelExists(_store, deviceId, requestedChannel)) {
            channel = static_cast<uint8_t>(requestedChannel);
            ok = _comms->sendSetState(deviceId, channel, desired);
            status = ok ? "sent" : "device_not_online";
        } else {
            status = valueValid ? "invalid_channel" : "invalid_arguments";
        }
    } else if (strcmp(command, "color") == 0) {
        int requestedChannel = doc["channel"] | 0;
        int color = doc["color"] | -1;
        const char *slot = doc["slot"] | "";
        if (!channelExists(_store, deviceId, requestedChannel)) {
            status = "invalid_channel";
        } else if (color >= 0 && color <= 7 &&
                   (strcmp(slot, "on") == 0 || strcmp(slot, "off") == 0)) {
            channel = static_cast<uint8_t>(requestedChannel);
            ok = _comms->sendSetColor(deviceId, channel, strcmp(slot, "on") == 0, color);
            status = ok ? "sent" : "device_not_online";
        } else {
            status = "invalid_arguments";
        }
    } else if (strcmp(command, "get_state") == 0) {
        int requestedChannel = doc["channel"] | 0;
        if (channelExists(_store, deviceId, requestedChannel)) {
            ok = _comms->sendGetState(deviceId, static_cast<uint8_t>(requestedChannel));
            status = ok ? "sent" : "device_not_online";
        } else {
            status = "invalid_channel";
        }
    } else if (strcmp(command, "name") == 0 || strcmp(command, "channel_name") == 0) {
        const char *name = doc["name"] | "";
        int requestedChannel = doc["channel"] | 0;
        bool channelName = strcmp(command, "channel_name") == 0;
        if (!doc["name"].is<const char *>() ||
                   strlen(name) >= (channelName ? MAX_CHANNEL_NAME_LEN : MAX_DEVICE_NAME_LEN)) {
            status = "invalid_name";
        } else if (channelName &&
                   (requestedChannel < 1 || requestedChannel > MAX_CHANNELS)) {
            status = "invalid_arguments";
        } else {
            DeviceRecord record = _store->fetchForUpdate(deviceId, DEVICE_TYPE_UNKNOWN);
            if (channelName) {
                strlcpy(record.channelNames[requestedChannel - 1], name,
                        sizeof(record.channelNames[requestedChannel - 1]));
            } else {
                strlcpy(record.name, name, sizeof(record.name));
            }
            _store->markDirty(record, millis());
            ok = true;
            status = "ok";
        }
    }

    publishAck(requestId, command, ok, status);
}

void MqttManager::tick(uint32_t nowMs) {
    if (!_network || !_network->mqttConfigured()) {
        disconnect();
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        disconnect();
        return;
    }

    if (!_mqtt || !_mqtt->connected()) {
        disconnect();
        if (!_hasConnectAttempted || nowMs - _lastConnectAttemptMs >= RECONNECT_INTERVAL_MS) {
            connect(nowMs);
        }
        return;
    }

    int messageSize = _mqtt->parseMessage();
    if (messageSize > 0) {
        String topic = _mqtt->messageTopic();
        bool retained = _mqtt->messageRetain() != 0;
        if (messageSize > MAX_COMMAND_BYTES || topic != _commandTopic) {
            while (_mqtt->available()) _mqtt->read();
        } else {
            char payload[MAX_COMMAND_BYTES + 1];
            size_t received = 0;
            while (received < static_cast<size_t>(messageSize)) {
                int byte = _mqtt->read();
                if (byte < 0) break;
                payload[received++] = static_cast<char>(byte);
            }
            payload[received] = '\0';
            if (received == static_cast<size_t>(messageSize)) {
                processCommand(payload, received, retained);
            } else {
                Serial.println("[MQTT] incomplete command payload");
            }
        }
    }

    if (nowMs - _lastStatePublishMs >= STATE_PUBLISH_INTERVAL_MS) {
        publishState(nowMs);
        _lastStatePublishMs = nowMs;
    }
}
