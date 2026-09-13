#include "DeviceStore.h"
#include <LittleFS.h>
#include <string.h>

static uint8_t computeChecksum(const DeviceRecord &r) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&r);
    uint16_t sum = 0;
    for (size_t i = 0; i < sizeof(DeviceRecord) - 1; i++) {
        sum += bytes[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

void DeviceStore::begin() {
    if (!LittleFS.begin(true)) { // true = format on first-ever boot / failed mount
        Serial.println("[DeviceStore] LittleFS mount failed");
        return;
    }
    if (!LittleFS.exists("/devices")) {
        LittleFS.mkdir("/devices");
    }
    for (uint8_t i = 0; i < MAX_PENDING_WRITES; i++) {
        _pending[i].used = false;
    }
}

String DeviceStore::pathFor(uint32_t deviceId) const {
    char name[24];
    snprintf(name, sizeof(name), "/devices/%08x.bin", deviceId);
    return String(name);
}

uint8_t DeviceStore::listKnownDeviceIds(uint32_t *outIds, uint8_t maxCount) {
    uint8_t count = 0;
    File dir = LittleFS.open("/devices");
    if (!dir || !dir.isDirectory()) return 0;

    File entry = dir.openNextFile();
    while (entry && count < maxCount) {
        String name = entry.name(); // filename only - content is never read here
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);

        if (name.endsWith(".bin")) {
            outIds[count] = strtoul(name.c_str(), nullptr, 16);
            count++;
        }
        entry.close();
        entry = dir.openNextFile();
    }
    return count;
}

void DeviceStore::defaultRecord(DeviceRecord &out, uint32_t deviceId, uint8_t deviceType) const {
    memset(&out, 0, sizeof(out));
    out.deviceId = deviceId;
    out.deviceType = deviceType;
}

bool DeviceStore::readFromFlash(uint32_t deviceId, DeviceRecord &out) {
    File f = LittleFS.open(pathFor(deviceId), "r");
    if (!f) return false;

    int got = f.read(reinterpret_cast<uint8_t *>(&out), sizeof(out));
    f.close();

    if (got != (int)sizeof(out)) return false;
    if (out.deviceId != deviceId) return false;
    if (computeChecksum(out) != out.checksum) return false;
    return true;
}

void DeviceStore::writeToFlash(const DeviceRecord &record) {
    DeviceRecord copy = record;
    copy.checksum = computeChecksum(copy);

    File f = LittleFS.open(pathFor(copy.deviceId), "w");
    if (!f) {
        Serial.println("[DeviceStore] flash write failed");
        return;
    }
    f.write(reinterpret_cast<const uint8_t *>(&copy), sizeof(copy));
    f.close();
}

int DeviceStore::findPending(uint32_t deviceId) const {
    for (uint8_t i = 0; i < MAX_PENDING_WRITES; i++) {
        if (_pending[i].used && _pending[i].deviceId == deviceId) return i;
    }
    return -1;
}

int DeviceStore::claimPendingSlot(uint32_t deviceId) {
    int existing = findPending(deviceId);
    if (existing >= 0) return existing;

    for (uint8_t i = 0; i < MAX_PENDING_WRITES; i++) {
        if (!_pending[i].used) {
            _pending[i].used = true;
            _pending[i].deviceId = deviceId;
            return i;
        }
    }

    // All slots busy (unlikely for a home hub) - flush the oldest to make
    // room rather than dropping the new change.
    uint8_t oldest = 0;
    for (uint8_t i = 1; i < MAX_PENDING_WRITES; i++) {
        if (_pending[i].dirtySinceMs < _pending[oldest].dirtySinceMs) oldest = i;
    }
    writeToFlash(_pending[oldest].record);
    _pending[oldest].deviceId = deviceId;
    return oldest; // .used stays true; caller sets dirtySinceMs right after
}

DeviceRecord DeviceStore::fetchForUpdate(uint32_t deviceId, uint8_t deviceType) {
    int pendingIndex = findPending(deviceId);
    if (pendingIndex >= 0) {
        return _pending[pendingIndex].record;
    }

    DeviceRecord record;
    if (readFromFlash(deviceId, record)) {
        return record;
    }

    defaultRecord(record, deviceId, deviceType);
    return record;
}

void DeviceStore::markDirty(const DeviceRecord &record, uint32_t nowMs) {
    int index = claimPendingSlot(record.deviceId);
    _pending[index].record = record;
    _pending[index].dirtySinceMs = nowMs; // (re)start the debounce window on every change
}

bool DeviceStore::getDetail(uint32_t deviceId, DeviceRecord &out) {
    int pendingIndex = findPending(deviceId);
    if (pendingIndex >= 0) {
        out = _pending[pendingIndex].record;
        return true;
    }
    return readFromFlash(deviceId, out);
}

void DeviceStore::tick(uint32_t nowMs) {
    for (uint8_t i = 0; i < MAX_PENDING_WRITES; i++) {
        if (_pending[i].used && nowMs - _pending[i].dirtySinceMs >= DEVICE_RECORD_SAVE_DELAY_MS) {
            writeToFlash(_pending[i].record);
            _pending[i].used = false;
        }
    }
}
