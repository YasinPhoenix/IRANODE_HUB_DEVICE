#ifndef IRANODE_PROTOCOL_H
#define IRANODE_PROTOCOL_H

#include <Arduino.h>

// ============================================================================
// IRANODE PROTOCOL v3
// Shared between the hub (ESP32) and every device (currently just the
// ESP8266 wall switch, more device types planned later - see deviceType
// below). This file is identical across every project - copy it as-is. If
// you change anything here, change it everywhere and bump
// IRANODE_PROTOCOL_VERSION so mismatched builds refuse to talk to each
// other instead of silently misreading a packet (iranodeValidatePacket()
// below already rejects a version mismatch, so this is a safety net, not
// just a formality).
//
// v3: added the deviceType field (see IranodeDeviceType below) ahead of
// the hub eventually handling device types other than the wall switch.
// ============================================================================

#define IRANODE_MAGIC_1 0x49
#define IRANODE_MAGIC_2 0x4E
#define IRANODE_PROTOCOL_VERSION 3

// ----------------------------------------------------------------------------
// What kind of physical device sent (or, for a hub->device command, is
// addressed to) this packet. The hub uses this to file a device in its
// registry and choose the right handling/UI logic for it - it is NOT part
// of the version-mismatch check, since a hub running ahead of a device's
// firmware should still be able to talk to a device type it already knows.
//
// Not implemented beyond WALL_SWITCH yet - reserving the tag now so adding
// a real device type later doesn't require another protocol-wide change.
// ----------------------------------------------------------------------------
enum IranodeDeviceType : uint8_t {
    DEVICE_TYPE_UNKNOWN     = 0,
    DEVICE_TYPE_WALL_SWITCH = 1
    // DEVICE_TYPE_SENSOR = 2, etc. go here when that project starts.
};

// ----------------------------------------------------------------------------
// Message types, and the payload convention each one carries.
//
// These are written from the wall switch's point of view, but most of them
// are generic on purpose: DISCOVERY_REQUEST/RESPONSE, HEARTBEAT, GET_STATE,
// STATE_REPORT/STATE_CHANGED, ACK and ERROR_MSG all just mean "device
// status" regardless of what the device actually is, so a future device
// type can reuse them as-is. SET_COLOR is genuinely switch/lighting
// specific - a device type that has no use for it simply never sends or
// handles it; that's fine, nothing else in the protocol assumes it will.
//
// `channel` is switch index + 1 (1..SWITCH_COUNT) for anything
// channel-specific; 0 means "not applicable" (discovery, heartbeat).
// ----------------------------------------------------------------------------
enum IranodeMessageType : uint8_t {

    // Hub -> broadcast. No payload.
    DISCOVERY_REQUEST  = 1,

    // Device -> Hub, reply to DISCOVERY_REQUEST.
    //   payload[0] = channel count (SWITCH_COUNT)
    //   payload[1] = firmware major
    //   payload[2] = firmware minor
    //   payloadLength = 3
    DISCOVERY_RESPONSE = 2,

    // Device -> Hub, every HEARTBEAT_INTERVAL_MS. No payload.
    HEARTBEAT          = 3,

    // Device -> Hub, unsolicited (a touch changed a relay/color).
    // Same payload layout as STATE_REPORT below.
    STATE_CHANGED      = 4,

    // Hub -> Device. Reply is a STATE_REPORT for that channel, or an ACK
    // carrying an error reason if channel/value is invalid.
    //   value = desired relay state (0/1)
    //   payloadLength = 0
    SET_STATE          = 5,

    // Device -> Hub. Error replies ONLY - a successful command gets a
    // STATE_REPORT back instead, never a bare ACK.
    //   value = 0 (always failure)
    //   payload[0] = IranodeErrorReason
    //   payloadLength = 1
    //   sequence = echoes the sequence number of the rejected packet
    ACK                = 6,

    // Hub -> Device. Reply is a STATE_REPORT for that channel.
    //   payloadLength = 0
    GET_STATE          = 7,

    // Device -> Hub. Sent once per channel on boot, and as the reply to
    // GET_STATE / SET_STATE / SET_COLOR for that channel.
    //   value = relay state (0/1)
    //   payload[0] = colorOn  (RGBColor enum value, 0-7)
    //   payload[1] = colorOff (RGBColor enum value, 0-7)
    //   payloadLength = 2
    STATE_REPORT       = 8,

    // Hub -> Device. Reply is a STATE_REPORT for that channel, or an ACK
    // carrying an error reason if channel/value is invalid.
    //   value = 0 -> set the off-color slot, 1 -> set the on-color slot
    //   payload[0] = color enum value (0-7)
    //   payloadLength = 1
    SET_COLOR          = 9,

    // Reserved, not used yet.
    ERROR_MSG          = 10
};

// Reason codes carried in an ACK's payload[0].
enum IranodeErrorReason : uint8_t {
    ERR_INVALID_CHANNEL = 1,
    ERR_INVALID_VALUE   = 2,
    ERR_UNKNOWN_TYPE    = 3
};

// ----------------------------------------------------------------------------
// Wire format - fixed size, packed, identical on both ends.
// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct IranodePacket {
    uint8_t  magic1;
    uint8_t  magic2;
    uint8_t  version;
    uint8_t  type;
    uint8_t  deviceType;

    uint16_t sequence;

    uint32_t deviceId;
    uint32_t uptimeSeconds;

    uint8_t  channel;
    uint8_t  value;

    uint8_t  payloadLength;
    uint8_t  payload[32];

    uint16_t crc;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
// CRC16/ARC - identical implementation required on both ends.
// ----------------------------------------------------------------------------
inline uint16_t iranodeCrc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        }
    }
    return crc;
}

// Call right before sending: stamps magic/version and (re)computes the CRC
// over everything except the CRC field itself.
inline void iranodePreparePacket(IranodePacket &packet) {
    packet.magic1  = IRANODE_MAGIC_1;
    packet.magic2  = IRANODE_MAGIC_2;
    packet.version = IRANODE_PROTOCOL_VERSION;
    packet.crc = iranodeCrc16(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(IranodePacket) - sizeof(packet.crc));
}

// Call on everything received, before touching packet.type. Rejects wrong
// magic, a protocol-version mismatch between hub/device builds, an
// oversized payloadLength, and a bad CRC - all in one check.
inline bool iranodeValidatePacket(const IranodePacket &packet) {
    if (packet.magic1 != IRANODE_MAGIC_1) return false;
    if (packet.magic2 != IRANODE_MAGIC_2) return false;
    if (packet.version != IRANODE_PROTOCOL_VERSION) return false;
    if (packet.payloadLength > sizeof(packet.payload)) return false;

    uint16_t receivedCrc = packet.crc;
    uint16_t calculatedCrc = iranodeCrc16(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(IranodePacket) - sizeof(packet.crc));

    return receivedCrc == calculatedCrc;
}

#endif // IRANODE_PROTOCOL_H
