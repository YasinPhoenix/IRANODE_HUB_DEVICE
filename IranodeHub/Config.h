#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// WIFI ACCESS POINT
// The hub hosts the network; every device joins it as a station. IPs are
// plain strings, not IPAddress, so files that don't need networking
// (DeviceStore) aren't forced to pull WiFi headers in through this shared
// header - same reasoning as the switch project's Config.h.
//
// SSID/password/max-connection-count are no longer fixed here - they're
// user-configurable and persisted by WifiApManager (see WifiApManager.h).
// AP_SSID_PREFIX is what an out-of-the-box, not-yet-configured hub's AP is
// named: "IranodeHub-<8 hex digit id>", the id coming from the hub's own
// WiFi MAC (see hubDeviceId() in WifiApManager.h). The hub's IP/subnet stay
// fixed even across a reconfiguration - only SSID/password/max connections
// are ever user-editable - so every other module that already assumes
// 192.168.4.1 (CommsManager, DeviceStore's peers, the switch project's
// HUB_IP_STR) keeps working unmodified.
// ============================================================================
#define AP_SSID_PREFIX "IranodeHub-"

#define AP_MAX_SSID_LEN     32
#define AP_MAX_PASSWORD_LEN 64
#define AP_MIN_PASSWORD_LEN 8   // matches WPA2's minimum; 0 (empty/open) is also allowed
#define AP_MIN_MAX_CONN     1
#define AP_MAX_MAX_CONN     10  // ESP32 SoftAP hard ceiling
#define AP_DEFAULT_MAX_CONN 8

#define AP_CONFIG_SAVE_PATH   "/config/ap.bin"
#define AP_RESTART_DELAY_MS   1200UL // lets the HTTP response reach the browser before ESP.restart()

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
