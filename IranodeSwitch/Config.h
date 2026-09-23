#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// SWITCH COUNT SELECTION
// Uncomment exactly one. Relay count, RGB LED count and touch-sensor count
// are always equal to SWITCH_COUNT - there is no separate/fixed sensor count.
// ============================================================================
// #define SWITCH_COUNT 1
// #define SWITCH_COUNT 2
// #define SWITCH_COUNT 3
#define SWITCH_COUNT 4

// ============================================================================
// PER-SWITCH-COUNT HARDWARE MAPPING
// Carried over unchanged from WallSwitch-ESP8266.
//
// IMPORTANT: two unrelated numbering spaces appear below. Do not compare
// them to each other:
//   - SR_DATA_PIN / SR_CLOCK_PIN / SR_LATCH_PIN / TOUCH_PINS are real GPIO
//     pin numbers (electrical connections).
//   - RELAY_BITS / RED_BITS / GREEN_BITS / BLUE_BITS are bit indices (0-15)
//     inside the 16-bit word shifted out to the two 74HC595s.
// ============================================================================

#if SWITCH_COUNT == 1
    #define SR_DATA_PIN  13
    #define SR_CLOCK_PIN 16
    #define SR_LATCH_PIN 14
    static const uint8_t TOUCH_PINS[SWITCH_COUNT] = {5};
    static const uint8_t RELAY_BITS[SWITCH_COUNT] = {10};
    static const uint8_t RED_BITS[SWITCH_COUNT]   = {4};
    static const uint8_t GREEN_BITS[SWITCH_COUNT] = {6};
    static const uint8_t BLUE_BITS[SWITCH_COUNT]  = {5};

#elif SWITCH_COUNT == 2
    #define SR_DATA_PIN  13
    #define SR_CLOCK_PIN 16
    #define SR_LATCH_PIN 14
    static const uint8_t TOUCH_PINS[SWITCH_COUNT] = {4, 12};
    static const uint8_t RELAY_BITS[SWITCH_COUNT] = {11, 9};
    static const uint8_t RED_BITS[SWITCH_COUNT]   = {1, 12};
    static const uint8_t GREEN_BITS[SWITCH_COUNT] = {3, 14};
    static const uint8_t BLUE_BITS[SWITCH_COUNT]  = {2, 13};

#elif SWITCH_COUNT == 3
    #define SR_DATA_PIN  13
    #define SR_CLOCK_PIN 16
    #define SR_LATCH_PIN 14
    static const uint8_t TOUCH_PINS[SWITCH_COUNT] = {4, 5, 12};
    static const uint8_t RELAY_BITS[SWITCH_COUNT] = {11, 10, 9};
    static const uint8_t RED_BITS[SWITCH_COUNT]   = {1, 4, 12};
    static const uint8_t GREEN_BITS[SWITCH_COUNT] = {3, 6, 14};
    static const uint8_t BLUE_BITS[SWITCH_COUNT]  = {2, 5, 13};

#elif SWITCH_COUNT == 4
    // NOTE: on this variant SR_DATA_PIN is GPIO1 (hardware UART TX), because
    // GPIO13 is used for a touch sensor instead. This is a fixed PCB trace,
    // not a firmware choice - Serial logging must stay off (or be moved via
    // Serial.swap()) on this variant, or boot-time UART output will corrupt
    // the shift-register data line.
    #define SR_DATA_PIN  1
    #define SR_CLOCK_PIN 16
    #define SR_LATCH_PIN 14
    static const uint8_t TOUCH_PINS[SWITCH_COUNT] = {4, 5, 13, 12};
    static const uint8_t RELAY_BITS[SWITCH_COUNT] = {10, 9, 12, 11};
    static const uint8_t RED_BITS[SWITCH_COUNT]   = {8, 5, 0, 15};
    static const uint8_t GREEN_BITS[SWITCH_COUNT] = {3, 7, 1, 14};
    static const uint8_t BLUE_BITS[SWITCH_COUNT]  = {4, 6, 2, 13};

#else
    #error "SWITCH_COUNT must be 1, 2, 3 or 4"
#endif

// ============================================================================
// COLORS
// Each channel is ON/OFF only -> 8 possible combinations. COLOR_RGB_BITS
// gives bit0=Red, bit1=Green, bit2=Blue for each enum value, so
// ShiftRegister::setColor() is a table lookup instead of a branch chain.
// ============================================================================
enum RGBColor : uint8_t {
    COLOR_OFF = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_WHITE,
    COLOR_COUNT
};

static const uint8_t COLOR_RGB_BITS[COLOR_COUNT] = {
    0b000, // OFF
    0b001, // RED
    0b010, // GREEN
    0b100, // BLUE
    0b011, // YELLOW  = R+G
    0b110, // CYAN    = G+B
    0b101, // MAGENTA = R+B
    0b111  // WHITE   = R+G+B
};

// Applied on first boot / invalid EEPROM: green while on, red while off.
#define DEFAULT_COLOR_ON  COLOR_BLUE
#define DEFAULT_COLOR_OFF COLOR_WHITE

// ============================================================================
// TIMING - local/physical behavior
// ============================================================================
#define TOUCH_DEBOUNCE_MS      50UL
#define RELAY_SAVE_DELAY_MS    300UL    // relay state: short, must survive a quick power loss
#define COLOR_SAVE_DELAY_MS    3000UL   // color prefs: longer, coalesces UI fiddling
#define FACTORY_RESET_HOLD_MS  10000UL  // hold every touch at once this long to wipe EEPROM + restart

// ============================================================================
// FIRMWARE VERSION
// Reported to the hub in DISCOVERY_RESPONSE.
// ============================================================================
#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0

// ============================================================================
// HUB CONNECTION
// This device is a WiFi STATION only - it joins the hub's access point, it
// never creates its own. There is no local web UI to configure this from
// (the hub is the only interface), so these are fixed at compile time.
//
// HUB_IP is a plain string, not an IPAddress, on purpose: keeping Config.h
// free of WiFi-library types means files that don't need networking
// (ShiftRegister, InputManager, State) don't drag WiFi headers in through
// this one shared header. HubLink.cpp turns it into a real IPAddress once,
// in the one file that actually needs it.
// ============================================================================
#define HUB_WIFI_SSID       "IRANODE-HUB"
#define HUB_WIFI_PASSWORD   "12345678"
#define HUB_IP_STR          "192.168.4.1"
#define HUB_PORT            4210
#define DISCOVERY_PORT      4211

#define HEARTBEAT_INTERVAL_MS      5000UL
#define WIFI_RECONNECT_INTERVAL_MS 5000UL  // how often to retry joining the hub while disconnected

// Gap between each channel's STATE_REPORT in the just-connected boot burst.
// Sending SWITCH_COUNT+1 UDP packets back-to-back right as WiFi comes up
// can outrun the radio/TCP-IP stack's TX queue before it's warmed up -
// spacing them out (still non-blocking, spread across tick() calls) avoids
// silently dropping one. 4-switch boards send 5 packets total in the
// burst and are the ones that showed this; 1-3 switch boards send fewer
// and were fine, which fits a queue-depth issue rather than a logic bug.
#define BOOT_REPORT_SPACING_MS     20UL

// What this project identifies itself as in Protocol.h's deviceType field.
// A different device project (e.g. a future sensor) defines its own value
// here - see IranodeDeviceType in Protocol.h.
#define IRANODE_DEVICE_TYPE DEVICE_TYPE_WALL_SWITCH

#endif // CONFIG_H