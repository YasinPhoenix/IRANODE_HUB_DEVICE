#ifndef HUB_LINK_H
#define HUB_LINK_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "Protocol.h"
#include "State.h"
#include "ShiftRegister.h"

// Owns the connection to the hub: joining its AP, the UDP socket, and every
// IranodePacket sent or received.
//
// Never blocks. begin() fires WiFi.begin() once and returns immediately;
// tick() just polls WiFi.status() every loop and reacts to whatever it
// sees. Nothing else in the firmware waits on this class for anything - a
// unit with no hub in range is a fully working wall switch from the moment
// StateManager::begin() runs, it just has nothing to report to yet.
class HubLink {
public:
    void begin(StateManager *state, ShiftRegister *sr);
    void tick(uint32_t nowMs);

    // Call right after a touch-triggered StateManager mutation, so the hub
    // hears about it immediately instead of waiting for the next heartbeat.
    // A no-op while disconnected - the burst sent on the next successful
    // connect already covers the current state of every channel.
    void reportStateChange(uint8_t index);

private:
    WiFiUDP _udp;
    StateManager *_state = nullptr;
    ShiftRegister *_sr = nullptr;

    IPAddress _hubIp;
    uint32_t _deviceId = 0;
    uint16_t _sequence = 0;

    bool _connected = false;
    uint32_t _lastReconnectAttemptMs = 0;
    uint32_t _lastHeartbeatMs = 0;

    void connectWifi();
    void onConnected();

    void processIncoming();
    void handleSetState(const IranodePacket &packet);
    void handleSetColor(const IranodePacket &packet);
    void handleGetState(const IranodePacket &packet);

    void sendDiscoveryResponse();
    void sendHeartbeat();
    void sendStateReport(uint8_t index, uint8_t type);
    void sendAck(uint16_t rejectedSequence, uint8_t reason);
    void sendPacketToHub(IranodePacket &packet);
};

#endif // HUB_LINK_H
