#include "DeviceRegistry.h"

static bool isBefore(const KnownDevice &a, const KnownDevice &b) {
    if (a.online != b.online) return a.online; // online devices sort first
    return a.lastSeenMs > b.lastSeenMs;         // then most recently seen
}

void DeviceRegistry::begin() {
    for (uint8_t i = 0; i < MAX_KNOWN_DEVICES; i++) {
        _devices[i].used = false;
    }
}

int DeviceRegistry::findSlot(uint32_t deviceId) const {
    for (uint8_t i = 0; i < MAX_KNOWN_DEVICES; i++) {
        if (_devices[i].used && _devices[i].deviceId == deviceId) return i;
    }
    return -1;
}

int DeviceRegistry::createSlot(uint32_t deviceId) {
    int existing = findSlot(deviceId);
    if (existing >= 0) return existing;

    for (uint8_t i = 0; i < MAX_KNOWN_DEVICES; i++) {
        if (!_devices[i].used) {
            _devices[i].used = true;
            _devices[i].deviceId = deviceId;
            _devices[i].ip = IPAddress(0, 0, 0, 0);
            _devices[i].online = false;
            _devices[i].lastSeenMs = 0;
            return i;
        }
    }
    return -1; // full - MAX_KNOWN_DEVICES needs raising for an installation this size
}

void DeviceRegistry::addKnown(uint32_t deviceId) {
    createSlot(deviceId);
}

void DeviceRegistry::noteSeen(uint32_t deviceId, IPAddress ip, uint32_t nowMs) {
    int index = createSlot(deviceId);
    if (index < 0) return;
    _devices[index].ip = ip;
    _devices[index].online = true;
    _devices[index].lastSeenMs = nowMs;
}

void DeviceRegistry::checkOffline(uint32_t nowMs) {
    for (uint8_t i = 0; i < MAX_KNOWN_DEVICES; i++) {
        if (_devices[i].used && _devices[i].online &&
            nowMs - _devices[i].lastSeenMs > DEVICE_OFFLINE_TIME) {
            _devices[i].online = false;
        }
    }
}

bool DeviceRegistry::find(uint32_t deviceId, KnownDevice &out) const {
    int index = findSlot(deviceId);
    if (index < 0) return false;
    out = _devices[index];
    return true;
}

uint8_t DeviceRegistry::sortedIndices(uint8_t *outIndices, uint8_t maxCount) const {
    uint8_t n = 0;
    for (uint8_t i = 0; i < MAX_KNOWN_DEVICES && n < maxCount; i++) {
        if (_devices[i].used) outIndices[n++] = i;
    }

    // Plain insertion sort - n is a home hub's device count, never large
    // enough to need anything fancier.
    for (uint8_t i = 1; i < n; i++) {
        uint8_t key = outIndices[i];
        int j = i - 1;
        while (j >= 0 && isBefore(_devices[key], _devices[outIndices[j]])) {
            outIndices[j + 1] = outIndices[j];
            j--;
        }
        outIndices[j + 1] = key;
    }
    return n;
}
