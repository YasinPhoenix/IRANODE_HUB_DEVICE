#include <WiFi.h>
#include "Config.h"
#include "DeviceRegistry.h"
#include "DeviceStore.h"
#include "CommsManager.h"
#include "WebApi.h"

DeviceRegistry deviceRegistry;
DeviceStore deviceStore;
CommsManager commsManager;
WebApi webApi;

void setup() {
    Serial.begin(115200);

    // 1. Host the network - every device joins this AP as a station.
    WiFi.mode(WIFI_AP);
    IPAddress hubIp, subnet;
    hubIp.fromString(HUB_IP_STR);
    subnet.fromString(SUBNET_STR);
    WiFi.softAPConfig(hubIp, hubIp, subnet);
    WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 10);

    // 2. Bring up storage and seed the known-device list from filenames
    //    only - no file content is read at boot (see DeviceStore). Every
    //    device the hub has ever seen shows up in the dashboard right
    //    away, marked offline, before any UDP traffic even arrives.
    deviceStore.begin();
    deviceRegistry.begin();
    uint32_t knownIds[MAX_KNOWN_DEVICES];
    uint8_t knownCount = deviceStore.listKnownDeviceIds(knownIds, MAX_KNOWN_DEVICES);
    for (uint8_t i = 0; i < knownCount; i++) {
        deviceRegistry.addKnown(knownIds[i]);
    }

    // 3. Comms (opens the UDP socket, sends the first discovery broadcast)
    //    and the dashboard.
    commsManager.begin(&deviceRegistry, &deviceStore);
    webApi.begin(&deviceRegistry, &deviceStore, &commsManager);

    Serial.print("IRANODE HUB up, ");
    Serial.print(knownCount);
    Serial.println(" known device(s) loaded from flash");
}

void loop() {
    uint32_t now = millis();
    commsManager.tick(now);
    webApi.tick();
}
