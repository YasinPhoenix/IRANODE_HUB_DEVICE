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
    _server.on("/api/device/name", HTTP_POST, [this]() { handlePostName(); });
    _server.on("/api/device/channel-name", HTTP_POST, [this]() { handlePostChannelName(); });
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

// Names are free text a person typed in, unlike every other value this API
// emits - so unlike the rest of this file's JSON building, this one path
// actually needs proper escaping, or a name containing a quote or
// backslash would corrupt the response.
static void appendJsonString(String &json, const char *value) {
    json += "\"";
    for (const char *p = value; *p; p++) {
        char c = *p;
        switch (c) {
            case '"':  json += "\\\""; break;
            case '\\': json += "\\\\"; break;
            case '\n': json += "\\n"; break;
            case '\r': json += "\\r"; break;
            case '\t': json += "\\t"; break;
            default:
                if ((uint8_t)c < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
                    json += buf;
                } else {
                    json += c;
                }
        }
    }
    json += "\"";
}

void WebApi::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

// Appends everything after the opening "{" that appendDeviceJson/
// handleDeviceDetail already wrote - i.e. name/type/fw/channels plus the
// type-specific object. Caller owns the surrounding braces.
void WebApi::appendRecordJson(String &json, const DeviceRecord &record) {
    json += "\"name\":";
    appendJsonString(json, record.name);
    json += ",\"type\":";
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
        for (uint8_t i = 0; i < record.channelCount && i < MAX_CHANNELS; i++) {
            if (i) json += ",";
            json += "{\"relay\":";
            json += (sw->relayStates & (1 << i)) ? "true" : "false";
            json += ",\"colorOn\":";
            json += String(sw->colorOn[i]);
            json += ",\"colorOff\":";
            json += String(sw->colorOff[i]);
            json += ",\"name\":";
            appendJsonString(json, record.channelNames[i]);
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
// actually clicks one. A device's name is part of that detail, so an
// offline device shows its raw hex id in the list until expanded.
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

// Renaming never touches the device over the wire - it's a hub-only label,
// so unlike relay/color it works whether or not the device is currently
// online. It's still rejected for a device the hub has genuinely never
// heard of, since there'd be nothing sensible to attach the name to.
void WebApi::handlePostName() {
    uint32_t id;
    if (!_server.hasArg("id") || !_server.hasArg("name") || !parseHexId(_server.arg("id"), id)) {
        _server.send(400, "text/plain", "missing or bad arguments");
        return;
    }
    KnownDevice device;
    if (!_registry->find(id, device)) {
        _server.send(404, "text/plain", "unknown device");
        return;
    }

    DeviceRecord record = _store->fetchForUpdate(id, DEVICE_TYPE_UNKNOWN);
    _server.arg("name").toCharArray(record.name, MAX_DEVICE_NAME_LEN);
    _store->markDirty(record, millis());
    _server.send(200, "text/plain", "ok");
}

void WebApi::handlePostChannelName() {
    uint32_t id;
    if (!_server.hasArg("id") || !_server.hasArg("channel") || !_server.hasArg("name") ||
        !parseHexId(_server.arg("id"), id)) {
        _server.send(400, "text/plain", "missing or bad arguments");
        return;
    }
    uint8_t channel = _server.arg("channel").toInt();
    if (channel < 1 || channel > MAX_CHANNELS) {
        _server.send(400, "text/plain", "bad channel");
        return;
    }
    KnownDevice device;
    if (!_registry->find(id, device)) {
        _server.send(404, "text/plain", "unknown device");
        return;
    }

    DeviceRecord record = _store->fetchForUpdate(id, DEVICE_TYPE_UNKNOWN);
    _server.arg("name").toCharArray(record.channelNames[channel - 1], MAX_CHANNEL_NAME_LEN);
    _store->markDirty(record, millis());
    _server.send(200, "text/plain", "ok");
}
