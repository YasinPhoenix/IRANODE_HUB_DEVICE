#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// WIFI ACCESS POINT
// The hub hosts the network; every device joins it as a station. IPs are
// plain strings, not IPAddress, so files that don't need networking
// (DeviceStore) aren't forced to pull WiFi headers in through this shared
// header - same reasoning as the switch project's Config.h.
// ============================================================================
#define AP_SSID     "IRANODE-HUB"
#define AP_PASSWORD "12345678"

#define HUB_IP_STR           "192.168.4.1"
#define SUBNET_STR           "255.255.255.0"
#define HUB_BROADCAST_IP_STR "192.168.4.255"

// ============================================================================
// UDP
// Devices bind their listening socket to DISCOVERY_PORT and use it for
// everything they receive - both the broadcast DISCOVERY_REQUEST and any
// unicast command. The hub binds to HUB_PORT for everything it receives.
// This asymmetry is intentional, carried over unchanged from the MVP.
// ============================================================================
#define HUB_PORT       4210
#define DISCOVERY_PORT 4211

#define DISCOVERY_INTERVAL  15000UL
#define DEVICE_OFFLINE_TIME 15000UL

// ============================================================================
// DEVICE REGISTRY
// Bounds the lightweight in-RAM list (id/ip/online/lastSeen only, ~13
// bytes/entry) - NOT how many devices flash can hold. Raise freely if a
// real installation ever has more devices than this.
// ============================================================================
#define MAX_KNOWN_DEVICES 32

// ============================================================================
// DEVICE RECORD PERSISTENCE (LittleFS)
// MAX_PENDING_WRITES bounds concurrently-debouncing devices, not the total
// device count - a handful is plenty, since realistically only a few
// devices change state within any given debounce window.
// ============================================================================
#define MAX_PENDING_WRITES          8
#define DEVICE_RECORD_SAVE_DELAY_MS 2000UL

// ============================================================================
// OUTGOING COMMAND RELIABILITY
// A SET_STATE/SET_COLOR/GET_STATE gets resent if nothing is heard back
// within COMMAND_TIMEOUT_MS, up to COMMAND_MAX_RETRIES times.
// ============================================================================
#define MAX_PENDING_COMMANDS 8
#define COMMAND_TIMEOUT_MS   500UL
#define COMMAND_MAX_RETRIES  2

#endif // CONFIG_H
