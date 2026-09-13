#ifndef WEB_API_H
#define WEB_API_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "DeviceRegistry.h"
#include "DeviceStore.h"
#include "CommsManager.h"

class WebApi {
public:
    void begin(DeviceRegistry *registry, DeviceStore *store, CommsManager *comms);
    void tick(); // pumps the underlying WebServer

private:
    WebServer _server{80};
    DeviceRegistry *_registry = nullptr;
    DeviceStore *_store = nullptr;
    CommsManager *_comms = nullptr;

    void handleRoot();
    void handleDeviceList();
    void handleDeviceDetail();
    void handlePostRelay();
    void handlePostColor();

    // Appends this device's type-specific object (e.g. "switch":{...}) to
    // json. A future device type adds its own branch here - nothing else
    // in this file changes.
    void appendRecordJson(String &json, const DeviceRecord &record);
    void appendDeviceJson(String &json, const KnownDevice &device, bool includeDetail);

    static String hexId(uint32_t id);
    static bool parseHexId(const String &str, uint32_t &out);
};

#endif // WEB_API_H
