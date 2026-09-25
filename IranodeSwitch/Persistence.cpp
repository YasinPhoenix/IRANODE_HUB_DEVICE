#include "Persistence.h"
#include <EEPROM.h>
#include <string.h>

static uint8_t computeChecksum(const PersistentState &s) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&s);
    uint16_t sum = 0;
    // Every byte except the checksum field itself, which is always last.
    for (size_t i = 0; i < sizeof(PersistentState) - 1; i++) {
        sum += bytes[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

static uint8_t computeWifiChecksum(const WifiCredentials &s) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&s);
    uint16_t sum = 0;
    for (size_t i = 0; i < sizeof(WifiCredentials) - 1; i++) {
        sum += bytes[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

void Persistence::begin() {
    EEPROM.begin(sizeof(PersistentState) + sizeof(WifiCredentials));
}

void Persistence::applyDefaults(PersistentState &out) {
    memset(&out, 0, sizeof(out));
    out.magic = PERSIST_MAGIC;
    out.version = PERSIST_VERSION;
    out.relayStates = 0; // all relays off
    for (uint8_t i = 0; i < 4; i++) {
        out.colorOn[i]  = DEFAULT_COLOR_ON;
        out.colorOff[i] = DEFAULT_COLOR_OFF;
    }
    out.checksum = computeChecksum(out);
}

bool Persistence::load(PersistentState &out) {
    EEPROM.get(0, out);
    if (out.magic != PERSIST_MAGIC) return false;
    if (out.version != PERSIST_VERSION) return false;
    if (computeChecksum(out) != out.checksum) return false;
    return true;
}

void Persistence::save(const PersistentState &in) {
    PersistentState copy = in;
    copy.magic = PERSIST_MAGIC;
    copy.version = PERSIST_VERSION;
    copy.checksum = computeChecksum(copy);
    EEPROM.put(0, copy);
    EEPROM.commit();
}

void Persistence::invalidate() {
    uint8_t bad = static_cast<uint8_t>(PERSIST_MAGIC ^ 0xFF); // anything != PERSIST_MAGIC
    EEPROM.put(0, bad); // magic is the struct's first byte - a 1-byte write is enough
    EEPROM.commit();
}

bool Persistence::loadWifi(WifiCredentials &out) {
    EEPROM.get(WIFI_EEPROM_OFFSET, out);
    if (out.magic != WIFI_PERSIST_MAGIC) return false;
    if (out.version != WIFI_PERSIST_VERSION) return false;
    if (computeWifiChecksum(out) != out.checksum) return false;
    return true;
}

void Persistence::saveWifi(const WifiCredentials &in) {
    WifiCredentials copy = in;
    copy.magic = WIFI_PERSIST_MAGIC;
    copy.version = WIFI_PERSIST_VERSION;
    copy.checksum = computeWifiChecksum(copy);
    EEPROM.put(WIFI_EEPROM_OFFSET, copy);
    EEPROM.commit();
}

void Persistence::invalidateWifi() {
    uint8_t bad = static_cast<uint8_t>(WIFI_PERSIST_MAGIC ^ 0xFF);
    EEPROM.put(WIFI_EEPROM_OFFSET, bad); // magic is the struct's first byte
    EEPROM.commit();
}
