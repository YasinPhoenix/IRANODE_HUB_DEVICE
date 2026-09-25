#ifndef WEB_API_H
#define WEB_API_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "DeviceRegistry.h"
#include "DeviceStore.h"
#include "CommsManager.h"
#include "WifiApManager.h"

class WebApi {
public:
    void begin(DeviceRegistry *registry, DeviceStore *store, CommsManager *comms, WifiApManager *wifiApManager);
    void tick(); // pumps the underlying WebServer

private:
    WebServer _server{80};
    DeviceRegistry *_registry = nullptr;
    DeviceStore *_store = nullptr;
    CommsManager *_comms = nullptr;
    WifiApManager *_wifiApManager = nullptr;

    void handleRoot();
    void handleDeviceList();
    void handleDeviceDetail();
    void handlePostRelay();
    void handlePostColor();
    void handlePostName();
    void handlePostChannelName();

    void handleWifiConfigPage();
    void handleGetWifiConfig();
    void handlePostWifiConfig();

    // Sends 403 and returns false if the hub isn't configured yet - guards
    // every device-facing route (list/detail/relay/color/name/channel-
    // name) so the only thing reachable in the unconfigured/default-AP
    // state is the AP configuration page and its API, per spec: "the user
    // should only be able to configure the AP."
    bool requireConfigured();

    // Appends this device's type-specific object (e.g. "switch":{...}) to
    // json, plus the universal name fields. A future device type adds its
    // own branch here - nothing else in this file changes.
    void appendRecordJson(String &json, const DeviceRecord &record);
    void appendDeviceJson(String &json, const KnownDevice &device, bool includeDetail);

    static String hexId(uint32_t id);
    static bool parseHexId(const String &str, uint32_t &out);
};

#endif // WEB_API_H
