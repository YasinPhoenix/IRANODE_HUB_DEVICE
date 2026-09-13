#ifndef COMMS_MANAGER_H
#define COMMS_MANAGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <IPAddress.h>
#include "Config.h"
#include "Protocol.h"
#include "DeviceRegistry.h"
#include "DeviceStore.h"

class CommsManager {
public:
    void begin(DeviceRegistry *registry, DeviceStore *store);
    void tick(uint32_t nowMs);

    // Each returns false immediately, with nothing sent, if deviceId isn't
    // currently known to be online - there's no address to send it to.
    bool sendSetState(uint32_t deviceId, uint8_t channel, bool value);
    bool sendSetColor(uint32_t deviceId, uint8_t channel, bool onSlot, uint8_t color);
    bool sendGetState(uint32_t deviceId, uint8_t channel);

private:
    WiFiUDP _udp;
    DeviceRegistry *_registry = nullptr;
    DeviceStore *_store = nullptr;

    IPAddress _broadcastIp;
    uint16_t _sequence = 0;
    uint32_t _lastDiscoveryMs = 0;

    struct PendingCommand {
        bool used;
        uint32_t deviceId;
        uint8_t channel;
        uint16_t sequence;
        IranodePacket packet; // kept verbatim so a retry can resend it as-is
        uint32_t sentAtMs;
        uint8_t retriesLeft;
    };
    PendingCommand _pendingCommands[MAX_PENDING_COMMANDS];

    void sendDiscoveryBroadcast();
    void processIncoming(uint32_t nowMs);

    void handleDiscoveryResponse(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs);
    void handleHeartbeat(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs);
    void handleStateReport(const IranodePacket &packet, IPAddress remoteIp, uint32_t nowMs);
    void handleAck(const IranodePacket &packet);

    bool sendToDevice(uint32_t deviceId, IranodePacket &packet);
    void registerPending(uint32_t deviceId, uint8_t channel, const IranodePacket &packet, uint32_t nowMs);
    void clearPendingFor(uint32_t deviceId, uint8_t channel);
    void checkRetries(uint32_t nowMs);
};

#endif // COMMS_MANAGER_H
