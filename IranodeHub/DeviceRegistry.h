#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <Arduino.h>
#include <IPAddress.h>
#include "Config.h"

// The lightweight, always-in-RAM fact of which devices exist and whether
// they're currently reachable. Deliberately does NOT hold per-channel
// state, firmware version, or anything else "detail"-shaped - see
// DeviceStore for that. Kept this thin specifically so the known-device
// list can grow for the entire life of the hub without RAM usage growing
// in step with it.
struct KnownDevice {
    bool      used;
    uint32_t  deviceId;
    IPAddress ip;         // needed to unicast commands - meaningless until online at least once
    bool      online;
    uint32_t  lastSeenMs; // 0 = not seen yet this session
};

class DeviceRegistry {
public:
    void begin();

    // Boot-time seeding only: registers deviceId as known-but-not-yet-
    // seen-this-session. Does not mark it online or touch its IP.
    void addKnown(uint32_t deviceId);

    // Called on every validated incoming packet. Creates the entry if this
    // deviceId has genuinely never been seen before (brand new device),
    // marks it online, and records its IP and lastSeenMs.
    void noteSeen(uint32_t deviceId, IPAddress ip, uint32_t nowMs);

    // Sweeps for devices that have gone quiet longer than
    // DEVICE_OFFLINE_TIME and marks them offline.
    void checkOffline(uint32_t nowMs);

    bool find(uint32_t deviceId, KnownDevice &out) const;
    const KnownDevice &at(uint8_t index) const { return _devices[index]; }

    // Fills outIndices with every used slot's index, sorted online-first
    // (then most-recently-seen first within each group). Returns how many
    // were written.
    uint8_t sortedIndices(uint8_t *outIndices, uint8_t maxCount) const;

private:
    KnownDevice _devices[MAX_KNOWN_DEVICES];

    int findSlot(uint32_t deviceId) const;
    int createSlot(uint32_t deviceId);
};

#endif // DEVICE_REGISTRY_H
