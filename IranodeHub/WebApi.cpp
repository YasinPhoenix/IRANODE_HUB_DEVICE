#include "WebApi.h"
#include "WebUI.h"

void WebApi::begin(DeviceRegistry *registry, DeviceStore *store, CommsManager *comms) {
    _registry = registry;
    _store = store;
    _comms = comms;

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/devices", HTTP_GET, [this]() { handleDeviceList(); });
    _server.on("/api/device/detail", HTTP_GET, [this]() { handleDeviceDetail(); });
    _server.on("/api/device/relay", HTTP_POST, [this]() { handlePostRelay(); });
    _server.on("/api/device/color", HTTP_POST, [this]() { handlePostColor(); });
    _server.begin();
}

void WebApi::tick() {
    _server.handleClient();
}

String WebApi::hexId(uint32_t id) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%08x", id);
    return String(buf);
}

bool WebApi::parseHexId(const String &str, uint32_t &out) {
    if (str.length() == 0) return false;
    out = strtoul(str.c_str(), nullptr, 16);
    return true;
}

void WebApi::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

// Appends everything after the opening "{" that appendDeviceJson/
// handleDeviceDetail already wrote - i.e. type/fw/channels plus the
// type-specific object. Caller owns the surrounding braces.
void WebApi::appendRecordJson(String &json, const DeviceRecord &record) {
    json += "\"type\":";
    json += String(record.deviceType);
    json += ",\"fw\":\"";
    json += String(record.fwMajor);
    json += ".";
    json += String(record.fwMinor);
    json += "\",\"channels\":";
    json += String(record.channelCount);

    if (record.deviceType == DEVICE_TYPE_WALL_SWITCH) {
        const WallSwitchTypeData *sw = reinterpret_cast<const WallSwitchTypeData *>(record.typeData);
        json += ",\"switch\":{\"channels\":[";
        for (uint8_t i = 0; i < record.channelCount && i < MAX_SWITCH_CHANNELS; i++) {
            if (i) json += ",";
            json += "{\"relay\":";
            json += (sw->relayStates & (1 << i)) ? "true" : "false";
            json += ",\"colorOn\":";
            json += String(sw->colorOn[i]);
            json += ",\"colorOff\":";
            json += String(sw->colorOff[i]);
            json += "}";
        }
        json += "]}";
    }
    // A future device type adds its own "else if (record.deviceType ==
    // ...)" here, appending its own key - nothing above needs to change.
}

void WebApi::appendDeviceJson(String &json, const KnownDevice &device, bool includeDetail) {
    json += "{\"id\":\"";
    json += hexId(device.deviceId);
    json += "\",\"online\":";
    json += device.online ? "true" : "false";

    if (device.lastSeenMs > 0) {
        json += ",\"lastSeenSec\":";
        json += String((millis() - device.lastSeenMs) / 1000);
    }

    if (includeDetail) {
        DeviceRecord record;
        if (_store->getDetail(device.deviceId, record)) {
            json += ",";
            appendRecordJson(json, record);
        }
    }
    json += "}";
}

// GET /api/devices - polled continuously. Sorted online-first. Offline
// entries are just {id, online:false[, lastSeenSec]} - full detail for
// those is only ever fetched lazily, via handleDeviceDetail(), when a user
// actually clicks one.
void WebApi::handleDeviceList() {
    uint8_t indices[MAX_KNOWN_DEVICES];
    uint8_t count = _registry->sortedIndices(indices, MAX_KNOWN_DEVICES);

    String json = "[";
    for (uint8_t i = 0; i < count; i++) {
        if (i) json += ",";
        const KnownDevice &device = _registry->at(indices[i]);
        appendDeviceJson(json, device, device.online); // only online devices carry inline detail
    }
    json += "]";

    _server.send(200, "application/json", json);
}

// GET /api/device/detail?id=<hex> - only called when a collapsed offline
// card is clicked.
void WebApi::handleDeviceDetail() {
    uint32_t id;
    if (!_server.hasArg("id") || !parseHexId(_server.arg("id"), id)) {
        _server.send(400, "application/json", "{\"found\":false}");
        return;
    }

    DeviceRecord record;
    if (!_store->getDetail(id, record)) {
        _server.send(200, "application/json", "{\"id\":\"" + hexId(id) + "\",\"found\":false}");
        return;
    }

    String json = "{\"id\":\"" + hexId(id) + "\",\"found\":true,";
    appendRecordJson(json, record);
    json += "}";
    _server.send(200, "application/json", json);
}

void WebApi::handlePostRelay() {
    uint32_t id;
    if (!_server.hasArg("id") || !_server.hasArg("channel") || !_server.hasArg("value") ||
        !parseHexId(_server.arg("id"), id)) {
        _server.send(400, "text/plain", "missing or bad arguments");
        return;
    }
    uint8_t channel = _server.arg("channel").toInt();
    bool value = _server.arg("value").toInt() != 0;

    bool ok = _comms->sendSetState(id, channel, value);
    _server.send(ok ? 200 : 409, "text/plain", ok ? "sent" : "device not online");
}

void WebApi::handlePostColor() {
    uint32_t id;
    if (!_server.hasArg("id") || !_server.hasArg("channel") ||
        !_server.hasArg("slot") || !_server.hasArg("color") ||
        !parseHexId(_server.arg("id"), id)) {
        _server.send(400, "text/plain", "missing or bad arguments");
        return;
    }
    uint8_t channel = _server.arg("channel").toInt();
    bool onSlot = _server.arg("slot").toInt() != 0;
    uint8_t color = _server.arg("color").toInt();

    bool ok = _comms->sendSetColor(id, channel, onSlot, color);
    _server.send(ok ? 200 : 409, "text/plain", ok ? "sent" : "device not online");
}
