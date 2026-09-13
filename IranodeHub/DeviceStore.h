#ifndef DEVICE_STORE_H
#define DEVICE_STORE_H

#include <Arduino.h>
#include "Config.h"
#include "Protocol.h" // IranodeDeviceType

#define MAX_SWITCH_CHANNELS 4

// On-flash (and, briefly, in-RAM while debounced) record for one device.
// Envelope fields are universal; typeData's meaning depends on deviceType -
// the same envelope + type-scoped-payload pattern as IranodePacket itself,
// so DeviceStore never needs to know what any particular device type is.
// Packed explicitly - deviceId's 4-byte alignment would otherwise let the
// compiler insert trailing padding, which would silently break the "sum
// every byte except the checksum" checksum convention used here and in
// the switch project's Persistence.h.
#pragma pack(push, 1)
struct DeviceRecord {
    uint32_t deviceId;
    uint8_t  deviceType;
    uint8_t  channelCount;
    uint8_t  fwMajor;
    uint8_t  fwMinor;
    uint8_t  typeData[16];
    uint8_t  checksum;
};

// DEVICE_TYPE_WALL_SWITCH overlays typeData as this. A future device type
// defines its own overlay for those same 16 bytes - nothing here or in
// DeviceStore.cpp needs to change when that happens.
struct WallSwitchTypeData {
    uint8_t relayStates; // bit i = channel i's relay state
    uint8_t colorOn[MAX_SWITCH_CHANNELS];
    uint8_t colorOff[MAX_SWITCH_CHANNELS];
};
#pragma pack(pop)

class DeviceStore {
public:
    void begin();

    // Directory listing only - fills outIds with every known device's ID
    // and returns how many were found. Never reads a file's content, by
    // design: this runs once at boot and must stay cheap regardless of how
    // many devices the hub has ever seen.
    uint8_t listKnownDeviceIds(uint32_t *outIds, uint8_t maxCount);

    // Starting point for a partial update: the current pending copy if
    // this device is already mid-debounce, else a flash read, else a
    // freshly zeroed record for a device that's never been recorded
    // before. Callers mutate only the fields they know about and pass the
    // result to markDirty() - this is what keeps DeviceStore itself free
    // of any per-device-type knowledge.
    DeviceRecord fetchForUpdate(uint32_t deviceId, uint8_t deviceType);

    // Registers `record` as this device's latest pending write and
    // (re)starts its debounce timer.
    void markDirty(const DeviceRecord &record, uint32_t nowMs);

    // Freshest known copy: a not-yet-flushed pending write if there is
    // one, otherwise a direct flash read. Never cached beyond the call -
    // this is deliberately safe to call on every poll for every online
    // device, and lazily on demand for an offline one.
    bool getDetail(uint32_t deviceId, DeviceRecord &out);

    // Flushes any pending write that has been dirty for at least
    // DEVICE_RECORD_SAVE_DELAY_MS.
    void tick(uint32_t nowMs);

private:
    struct PendingWrite {
        bool used;
        uint32_t deviceId;
        uint32_t dirtySinceMs;
        DeviceRecord record;
    };
    PendingWrite _pending[MAX_PENDING_WRITES];

    int findPending(uint32_t deviceId) const;
    int claimPendingSlot(uint32_t deviceId);

    String pathFor(uint32_t deviceId) const;
    bool readFromFlash(uint32_t deviceId, DeviceRecord &out);
    void writeToFlash(const DeviceRecord &record);
    void defaultRecord(DeviceRecord &out, uint32_t deviceId, uint8_t deviceType) const;
};

#endif // DEVICE_STORE_H
