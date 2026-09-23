#include "HubLink.h"

void HubLink::begin(StateManager *state, ShiftRegister *sr) {
    _state = state;
    _sr = sr;
    _deviceId = ESP.getChipId();
    _hubIp.fromString(HUB_IP_STR);

    WiFi.mode(WIFI_STA);
    connectWifi(); // non-blocking - returns immediately, connection happens in the background
}

void HubLink::connectWifi() {
    WiFi.begin(HUB_WIFI_SSID, HUB_WIFI_PASSWORD);
    _lastReconnectAttemptMs = millis();
}

void HubLink::tick(uint32_t nowMs) {
    bool isUp = (WiFi.status() == WL_CONNECTED);

    if (isUp && !_connected) {
        onConnected(); // just (re)joined the hub's AP
    } else if (!isUp && _connected) {
        _connected = false; // link just dropped - resume retry cadence below
    }

    if (!isUp) {
        // Deliberately not relying on the core's implicit auto-reconnect -
        // an explicit, timed retry keeps this deterministic regardless of
        // core version or why the link dropped (hub rebooted, out of
        // range, never paired yet).
        if (nowMs - _lastReconnectAttemptMs >= WIFI_RECONNECT_INTERVAL_MS) {
            connectWifi();
        }
        return; // nothing else to do while disconnected
    }

    processIncoming();
    tickBootReportBurst(nowMs);

    if (nowMs - _lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatMs = nowMs;
        sendHeartbeat();
    }
}

void HubLink::onConnected() {
    _udp.begin(DISCOVERY_PORT);
    _connected = true;
    _lastHeartbeatMs = millis();

    // Don't wait for the hub's next periodic DISCOVERY_REQUEST broadcast
    // (up to several seconds away) - announce immediately, then report
    // every channel's current state so the hub is fully in sync right away.
    // The per-channel reports are staggered via tickBootReportBurst()
    // rather than fired in a tight loop here - see BOOT_REPORT_SPACING_MS.
    sendDiscoveryResponse();
    _bootReportNext = 0;
    _lastBootReportMs = millis();
}

// Sends one boot-burst STATE_REPORT per call, no more often than
// BOOT_REPORT_SPACING_MS apart, until every channel has reported once.
// Non-blocking - just advances a little further on each tick().
void HubLink::tickBootReportBurst(uint32_t nowMs) {
    if (_bootReportNext >= SWITCH_COUNT) return; // burst already finished/inactive
    if (nowMs - _lastBootReportMs < BOOT_REPORT_SPACING_MS) return;

    sendStateReport(_bootReportNext, STATE_REPORT);
    _bootReportNext++;
    _lastBootReportMs = nowMs;
}

void HubLink::reportStateChange(uint8_t index) {
    sendStateReport(index, STATE_CHANGED);
}

void HubLink::processIncoming() {
    int packetSize = _udp.parsePacket();
    if (packetSize <= 0) return;

    if (packetSize != sizeof(IranodePacket)) {
        while (_udp.available()) _udp.read(); // drain and drop, wrong size
        return;
    }

    IranodePacket packet{};
    _udp.read(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

    if (!iranodeValidatePacket(packet)) return; // bad magic/version/crc - silently dropped

    switch (packet.type) {
        case DISCOVERY_REQUEST:
            sendDiscoveryResponse();
            break;
        case SET_STATE:
            handleSetState(packet);
            break;
        case SET_COLOR:
            handleSetColor(packet);
            break;
        case GET_STATE:
            handleGetState(packet);
            break;
        default:
            break; // not a command this device needs to react to
    }
}

void HubLink::handleSetState(const IranodePacket &packet) {
    uint8_t index = packet.channel - 1; // channel is 1-based; 0 underflows to 255, caught below
    if (packet.channel < 1 || index >= SWITCH_COUNT) {
        sendAck(packet.sequence, ERR_INVALID_CHANNEL);
        return;
    }
    if (packet.value != 0 && packet.value != 1) {
        sendAck(packet.sequence, ERR_INVALID_VALUE);
        return;
    }

    bool want = (packet.value == 1);
    if (_state->get(index).relayOn != want) {
        _state->toggleRelay(index, *_sr);
    }
    sendStateReport(index, STATE_REPORT); // doubles as the success acknowledgment
}

void HubLink::handleSetColor(const IranodePacket &packet) {
    uint8_t index = packet.channel - 1;
    if (packet.channel < 1 || index >= SWITCH_COUNT) {
        sendAck(packet.sequence, ERR_INVALID_CHANNEL);
        return;
    }
    if (packet.value != 0 && packet.value != 1) {
        sendAck(packet.sequence, ERR_INVALID_VALUE);
        return;
    }
    if (packet.payloadLength < 1 || packet.payload[0] >= COLOR_COUNT) {
        sendAck(packet.sequence, ERR_INVALID_VALUE);
        return;
    }

    bool forOnState = (packet.value == 1);
    _state->setColor(index, forOnState, packet.payload[0], *_sr);
    sendStateReport(index, STATE_REPORT);
}

void HubLink::handleGetState(const IranodePacket &packet) {
    uint8_t index = packet.channel - 1;
    if (packet.channel < 1 || index >= SWITCH_COUNT) {
        sendAck(packet.sequence, ERR_INVALID_CHANNEL);
        return;
    }
    sendStateReport(index, STATE_REPORT);
}

void HubLink::sendDiscoveryResponse() {
    IranodePacket packet{};
    packet.type = DISCOVERY_RESPONSE;
    packet.sequence = _sequence++;
    packet.deviceId = _deviceId;
    packet.uptimeSeconds = millis() / 1000;
    packet.channel = 0;
    packet.value = 0;
    packet.payload[0] = SWITCH_COUNT;
    packet.payload[1] = FW_VERSION_MAJOR;
    packet.payload[2] = FW_VERSION_MINOR;
    packet.payloadLength = 3;
    sendPacketToHub(packet);
}

void HubLink::sendHeartbeat() {
    IranodePacket packet{};
    packet.type = HEARTBEAT;
    packet.sequence = _sequence++;
    packet.deviceId = _deviceId;
    packet.uptimeSeconds = millis() / 1000;
    packet.channel = 0;
    packet.value = 0;
    packet.payloadLength = 0;
    sendPacketToHub(packet);
}

void HubLink::sendStateReport(uint8_t index, uint8_t type) {
    if (index >= SWITCH_COUNT) return;
    const SwitchState &s = _state->get(index);

    IranodePacket packet{};
    packet.type = type;
    packet.sequence = _sequence++;
    packet.deviceId = _deviceId;
    packet.uptimeSeconds = millis() / 1000;
    packet.channel = index + 1;
    packet.value = s.relayOn ? 1 : 0;
    packet.payload[0] = s.colorOn;
    packet.payload[1] = s.colorOff;
    packet.payloadLength = 2;
    sendPacketToHub(packet);
}

void HubLink::sendAck(uint16_t rejectedSequence, uint8_t reason) {
    IranodePacket packet{};
    packet.type = ACK;
    packet.sequence = rejectedSequence; // echo back so the hub can match it to its command
    packet.deviceId = _deviceId;
    packet.uptimeSeconds = millis() / 1000;
    packet.channel = 0;
    packet.value = 0;
    packet.payload[0] = reason;
    packet.payloadLength = 1;
    sendPacketToHub(packet);
}

void HubLink::sendPacketToHub(IranodePacket &packet) {
    if (!_connected) return; // nothing to send to right now
    packet.deviceType = IRANODE_DEVICE_TYPE;
    iranodePreparePacket(packet);
    _udp.beginPacket(_hubIp, HUB_PORT);
    _udp.write(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    _udp.endPacket();
}