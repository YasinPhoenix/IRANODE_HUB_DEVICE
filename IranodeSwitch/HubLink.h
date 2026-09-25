#ifndef HUB_LINK_H
#define HUB_LINK_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "Protocol.h"
#include "State.h"
#include "ShiftRegister.h"
#include "Persistence.h" // MAX_WIFI_SSID_LEN / MAX_WIFI_PASS_LEN

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
    // ssid/password are copied internally (fixed-size buffers, no String) -
    // connectWifi() re-issues WiFi.begin() with them on every reconnect
    // attempt, so the caller's buffers don't need to outlive this call.
    void begin(StateManager *state, ShiftRegister *sr, const char *ssid, const char *password);
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

    char _ssid[MAX_WIFI_SSID_LEN + 1];
    char _password[MAX_WIFI_PASS_LEN + 1];

    bool _connected = false;
    uint32_t _lastReconnectAttemptMs = 0;
    uint32_t _lastHeartbeatMs = 0;

    // Boot-time state-report burst, spread across tick() calls instead of
    // fired in one tight loop - see BOOT_REPORT_SPACING_MS in Config.h.
    // _bootReportNext == SWITCH_COUNT means the burst is done/inactive.
    uint8_t _bootReportNext = SWITCH_COUNT;
    uint32_t _lastBootReportMs = 0;

    void connectWifi();
    void onConnected();
    void tickBootReportBurst(uint32_t nowMs);

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