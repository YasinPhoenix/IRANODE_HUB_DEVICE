#include "CommsManager.h"

void CommsManager::begin(DeviceRegistry *registry, DeviceStore *store) {
    _registry = registry;
    _store = store;
    _broadcastIp.fromString(HUB_BROADCAST_IP_STR);

    _udp.begin(HUB_PORT);
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; i++) {
        _pendingCommands[i].used = false;
    }

    sendDiscoveryBroadcast();
}

void CommsManager::tick(uint32_t nowMs) {
    processIncoming(nowMs);
    _registry->checkOffline(nowMs);
    _store->tick(nowMs);
    checkRetries(nowMs);

    if (nowMs - _lastDiscoveryMs >= DISCOVERY_INTERVAL) {
        _lastDiscoveryMs = nowMs;
        sendDiscoveryBroadcast();
    }
}

void CommsManager::sendDiscoveryBroadcast() {
    IranodePacket packet{};
    packet.type = DISCOVERY_REQUEST;
    packet.sequence = _sequence++;
    packet.deviceId = 0; // not addressed to anyone in particular
    packet.uptimeSeconds = millis() / 1000;
    packet.channel = 0;
    packet.value = 0;
    packet.payloadLength = 0;

    iranodePreparePacket(packet);
    _udp.beginPacket(_broadcastIp, DISCOVERY_PORT);
    _udp.write(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    _udp.endPacket();
}

void CommsManager::processIncoming(uint32_t nowMs) {
    int packetSize = _udp.parsePacket();
    if (packetSize <= 0) return;

    if (packetSize != sizeof(IranodePacket)) {
        while (_udp.available()) _udp.read(); // drain and drop, wrong size
        return;
    }

    IranodePacket packet{};
    _udp.read(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    if (!iranodeValidatePacket(packet)) return;

    IPAddress remoteIp = _udp.remoteIP();

    switch (packet.type) {
        case DISCOVERY_RESPONSE:
            handleDiscoveryResponse(packet, remoteIp, nowMs);
            break;
        case HEARTBEAT:
            handleHeartbeat(packet, remoteIp, nowMs);
            break;
        case STATE_CHANGED:
        case STATE_REPORT:
            handleStateReport(packet, remoteIp, nowMs);
            break;
        case ACK:
            handleAck(packet);
            break;
        default:
            break;
    }
}

void CommsManager::handleDiscoveryResponse(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs) {
    if (packet.payloadLength < 3) return;

    _registry->noteSeen(packet.deviceId, remoteIp, nowMs);

    // Metadata-only update - typeData (relay/color etc.) is left exactly
    // as fetchForUpdate() found it. The state-report burst a device sends
    // right after connecting arrives within the same debounce window, so
    // this and the state update below typically coalesce into one flush.
    DeviceRecord record = _store->fetchForUpdate(packet.deviceId, packet.deviceType);
    record.channelCount = packet.payload[0];
    record.fwMajor = packet.payload[1];
    record.fwMinor = packet.payload[2];
    _store->markDirty(record, nowMs);
}

void CommsManager::handleHeartbeat(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs) {
    _registry->noteSeen(packet.deviceId, remoteIp, nowMs);
    // No DeviceStore write - a heartbeat carries no state, just liveness,
    // and writing on every one would be a flash write every 5s per device
    // for no new information.
}

void CommsManager::handleStateReport(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs) {
    _registry->noteSeen(packet.deviceId, remoteIp, nowMs);
    if (packet.channel < 1) return; // channel 0 means "not applicable", not a real index

    DeviceRecord record = _store->fetchForUpdate(packet.deviceId, packet.deviceType);

    if (packet.deviceType == DEVICE_TYPE_WALL_SWITCH && packet.payloadLength >= 2) {
        uint8_t index = packet.channel - 1;
        if (index < MAX_CHANNELS) {
            WallSwitchTypeData *sw = reinterpret_cast<WallSwitchTypeData *>(record.typeData);
            if (packet.value) sw->relayStates |= (1 << index);
            else              sw->relayStates &= ~(1 << index);
            sw->colorOn[index]  = packet.payload[0];
            sw->colorOff[index] = packet.payload[1];
        }
    }
    // A future device type's report handling goes here as its own "else
    // if (packet.deviceType == ...)" - nothing above needs to change.

    _store->markDirty(record, nowMs);
    clearPendingFor(packet.deviceId, packet.channel);
}

void CommsManager::handleAck(const IranodePacket &packet) {
    uint8_t reason = packet.payloadLength >= 1 ? packet.payload[0] : 0;
    Serial.print("[ACK] command rejected by 0x");
    Serial.print(packet.deviceId, HEX);
    Serial.print(" reason=");
    Serial.println(reason);

    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; i++) {
        if (_pendingCommands[i].used &&
            _pendingCommands[i].deviceId == packet.deviceId &&
            _pendingCommands[i].sequence == packet.sequence) {
            _pendingCommands[i].used = false; // device rejected it - stop retrying
            return;
        }
    }
}

void CommsManager::clearPendingFor(uint32_t deviceId, uint8_t channel) {
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; i++) {
        if (_pendingCommands[i].used &&
            _pendingCommands[i].deviceId == deviceId &&
            _pendingCommands[i].channel == channel) {
            _pendingCommands[i].used = false; // device is clearly responsive - good enough to stop retrying
        }
    }
}

bool CommsManager::sendToDevice(uint32_t deviceId, IranodePacket &packet) {
    KnownDevice device;
    if (!_registry->find(deviceId, device) || !device.online) return false;

    packet.deviceId = deviceId; // identifies which device this command targets
    packet.sequence = _sequence++;
    packet.uptimeSeconds = millis() / 1000;
    // packet.deviceType left at 0 (DEVICE_TYPE_UNKNOWN) - the hub has no
    // device-type identity of its own to report here.

    iranodePreparePacket(packet);
    _udp.beginPacket(device.ip, DISCOVERY_PORT); // devices listen on DISCOVERY_PORT, not HUB_PORT
    _udp.write(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    _udp.endPacket();

    registerPending(deviceId, packet.channel, packet, millis());
    return true;
}

void CommsManager::registerPending(uint32_t deviceId, uint8_t channel, const IranodePacket &packet, uint32_t nowMs) {
    int slot = -1;
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; i++) {
        if (!_pendingCommands[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        // All busy (unlikely) - overwrite the oldest rather than silently
        // drop the new command.
        slot = 0;
        for (uint8_t i = 1; i < MAX_PENDING_COMMANDS; i++) {
            if (_pendingCommands[i].sentAtMs < _pendingCommands[slot].sentAtMs) slot = i;
        }
    }
    _pendingCommands[slot].used = true;
    _pendingCommands[slot].deviceId = deviceId;
    _pendingCommands[slot].channel = channel;
    _pendingCommands[slot].sequence = packet.sequence;
    _pendingCommands[slot].packet = packet;
    _pendingCommands[slot].sentAtMs = nowMs;
    _pendingCommands[slot].retriesLeft = COMMAND_MAX_RETRIES;
}

void CommsManager::checkRetries(uint32_t nowMs) {
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; i++) {
        if (!_pendingCommands[i].used) continue;
        if (nowMs - _pendingCommands[i].sentAtMs < COMMAND_TIMEOUT_MS) continue;

        KnownDevice device;
        bool stillReachable = _registry->find(_pendingCommands[i].deviceId, device) && device.online;

        if (_pendingCommands[i].retriesLeft > 0 && stillReachable) {
            _udp.beginPacket(device.ip, DISCOVERY_PORT);
            _udp.write(reinterpret_cast<uint8_t *>(&_pendingCommands[i].packet), sizeof(IranodePacket));
            _udp.endPacket();
            _pendingCommands[i].retriesLeft--;
            _pendingCommands[i].sentAtMs = nowMs;
        } else {
            _pendingCommands[i].used = false; // gave up, or the device went offline mid-retry
        }
    }
}

bool CommsManager::sendSetState(uint32_t deviceId, uint8_t channel, bool value) {
    IranodePacket packet{};
    packet.type = SET_STATE;
    packet.channel = channel;
    packet.value = value ? 1 : 0;
    packet.payloadLength = 0;
    return sendToDevice(deviceId, packet);
}

bool CommsManager::sendSetColor(uint32_t deviceId, uint8_t channel, bool onSlot, uint8_t color) {
    IranodePacket packet{};
    packet.type = SET_COLOR;
    packet.channel = channel;
    packet.value = onSlot ? 1 : 0;
    packet.payload[0] = color;
    packet.payloadLength = 1;
    return sendToDevice(deviceId, packet);
}

bool CommsManager::sendGetState(uint32_t deviceId, uint8_t channel) {
    IranodePacket packet{};
    packet.type = GET_STATE;
    packet.channel = channel;
    packet.value = 0;
    packet.payloadLength = 0;
    return sendToDevice(deviceId, packet);
}
