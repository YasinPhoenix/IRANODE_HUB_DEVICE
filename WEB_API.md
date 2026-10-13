# Iranode Hub integration contract

## Topic namespace

The hub ID is the lowercase, eight-digit hexadecimal representation of the
lower 32 bits of its ESP32 eFuse MAC. The hub ID is part of each topic and is
**not** repeated in the state JSON payload. The server should associate a
received topic with its hub record before forwarding its state to the
corresponding mobile app/account.

```text
iranode/v1/{hubId}/availability
iranode/v1/{hubId}/state
iranode/v1/{hubId}/command
iranode/v1/{hubId}/ack
```

Example hub ID `1a2b3c4d`:

| Topic | Direction | QoS | Retain | Purpose |
|---|---|---:|---|---|
| `iranode/v1/1a2b3c4d/availability` | Hub → broker | 1 | Yes | Online state and Last Will. |
| `iranode/v1/1a2b3c4d/state` | Hub → broker | 1 | Yes | Full device snapshot. |
| `iranode/v1/1a2b3c4d/command` | Server → hub | 1 | **No** | JSON command request. |
| `iranode/v1/1a2b3c4d/ack` | Hub → broker | 1 | No | JSON command result, correlated by `requestId`. |

The hub subscribes only to its own `command` topic. A server should derive the
hub ID from its provisioning/registry data and publish commands to that exact
topic. Broker ACLs should allow the hub to publish `availability`, `state`,
and `ack`, and subscribe to `command`; the server needs the opposite
permissions for that hub's topics.

**Commands must be published with retain disabled.** A retained command could
be delivered again after reconnect and cause an old action to run. The hub
rejects messages it receives marked as retained, but MQTT 3.1.1 brokers can
clear the retain flag when forwarding a retained publish to an already-active
subscriber. Therefore the server/broker must enforce non-retained commands;
the hub's check is only an additional safeguard for replayed retained
deliveries, not a guarantee that all retained publishes are caught.

### Availability and Last Will

On connect the hub publishes this retained online message:

```json
{"online":true}
```

The broker Last Will is QoS 1, retained, and contains:

```json
{"online":false}
```

The hub also publishes retained offline availability before a graceful
disconnect when possible. The Last Will covers unexpected disconnections.

### Device state snapshot

`state` is a retained JSON array containing every device known to the hub,
including offline devices. It is sent on MQTT connect and then every five
seconds. If the hub disconnects, the broker retains the last state snapshot;
consumers should use `availability` to tell whether the hub is currently
connected. Devices whose detail record has not yet been received/saved contain
only their availability metadata. State shape:

```json
[
  {
    "id": "1a2b3c4d",
    "online": true,
    "lastSeenSec": 2,
    "name": "Living room",
    "type": 1,
    "fw": "1.2",
    "channels": 2,
    "switch": {
      "channels": [
        {"relay": true, "colorOn": 3, "colorOff": 7, "name": "Main"},
        {"relay": false, "colorOn": 3, "colorOff": 7, "name": "Lamp"}
      ]
    }
  },
  {
    "id": "5e6f7081",
    "online": false,
    "lastSeenSec": 48
  }
]
```

Device fields:

| Field | Type | Description |
|---|---|---|
| `id` | string | Eight lowercase hexadecimal device ID. |
| `online` | boolean | Whether the hub has heard from the device recently. The hub considers a device offline after more than 15 seconds without a packet. |
| `lastSeenSec` | integer, optional | Seconds since the last packet this hub boot. Omitted until the device has been seen this boot. |
| `name` | string | Hub-side device label. Empty string means no custom name. |
| `type` | integer | Device type: `1` wall switch, `0` unknown. |
| `fw` | string | Firmware version formatted as `"major.minor"`. |
| `channels` | integer | Number of channels reported by the device. |
| `switch` | object, conditional | Type-specific wall-switch details; has a `channels` array. |

Each `switch.channels[]` item contains `relay` (boolean), `colorOn` (integer),
`colorOff` (integer), and `name` (string). Color codes are `0` off, `1` red,
`2` green, `3` blue, `4` yellow, `5` cyan, `6` magenta, and `7` white.

The array order is not a stable identifier; servers must key devices by `id`
and channels by their one-based array position (`index + 1`).

### Command requests

Publish JSON to `iranode/v1/{hubId}/command` at QoS 1, without retain. Every
command requires a unique `requestId` for correlating its result, an operation
name in `command`, and the target device's eight-character hex `id`.

#### Set relay state

```json
{
  "requestId": "req-1001",
  "command": "relay",
  "id": "1a2b3c4d",
  "channel": 1,
  "value": true
}
```

`channel` is one-based and must refer to a channel reported for that device.
`value` is a JSON boolean (integer `0`/`1` is also accepted).

#### Set a color

```json
{
  "requestId": "req-1002",
  "command": "color",
  "id": "1a2b3c4d",
  "channel": 1,
  "slot": "on",
  "color": 3
}
```

`channel` must refer to a channel reported for that device. `slot` is `"on"`
or `"off"`; `color` is an integer from `0` through `7`.

#### Set device name

```json
{
  "requestId": "req-1003",
  "command": "name",
  "id": "1a2b3c4d",
  "name": "Living room"
}
```

#### Set channel name

```json
{
  "requestId": "req-1004",
  "command": "channel_name",
  "id": "1a2b3c4d",
  "channel": 1,
  "name": "Main"
}
```

Device and channel names are stored on the hub; they are not sent to the
physical switch. The current maximum lengths are 23 bytes for a device name
and 15 bytes for a channel name (limits are UTF-8 bytes, not Unicode
characters). An empty string clears a custom name. A channel-name command
accepts channel numbers `1`–`4`; the name is stored in that channel slot.

#### Request a fresh channel report

```json
{
  "requestId": "req-1005",
  "command": "get_state",
  "id": "1a2b3c4d",
  "channel": 1
}
```

This sends a request to the physical device. The device's subsequent state
report appears in the next retained snapshot. The ack confirms only that the
request was sent, not that a report has already arrived.

### Command acknowledgements

The hub publishes one acknowledgement to `iranode/v1/{hubId}/ack` for each
valid JSON command with a non-empty `requestId` and `command`:

```json
{
  "requestId": "req-1001",
  "command": "relay",
  "ok": true,
  "status": "sent"
}
```

`ok: true` and `status: "sent"` mean the hub accepted and sent a relay/color/
state request to an online device. They do **not** prove the physical change
has completed; use the state snapshot for observed device state. A successful
name update uses `status: "ok"`.

| `status` | Meaning |
|---|---|
| `invalid_device_id` | `id` is not exactly eight hexadecimal digits. |
| `unknown_device` | The hub has no registry entry for that device ID. |
| `invalid_arguments` | A command field is missing or has an unacceptable value. An out-of-range `channel_name` channel currently uses this status too. |
| `invalid_channel` | Channel is outside `1`–`4`, or the hub has no saved detail/channel count for validating it. |
| `invalid_name` | `name` is not a string or is too long for the relevant name field. |
| `invalid_command` | `command` is not one of `relay`, `color`, `get_state`, `name`, or `channel_name`. |
| `device_not_online` | A relay, color, or state request could not be sent because the device is offline. |
| `retained_command_rejected` | The broker delivered the command with its retain flag set. |

Invalid JSON or a message missing `requestId` or `command` is logged by the
hub and cannot be correlated, so no acknowledgement is sent. Commands are
validated against the hub's currently known devices. Physical-channel commands
(`relay`, `color`, `get_state`) also require saved channel-count detail. Wait
until a device's state snapshot includes its detail before enabling those
controls. `channel_name` uses the fixed supported slot range `1`–`4`.

MQTT QoS 1 is at-least-once delivery. The hub does not deduplicate
`requestId`s. Prefer absolute relay values and make server retries idempotent.
If the server retries a request, it may receive duplicate acknowledgements
and the physical command may be sent more than once.

### Mobile app/server integration guidance

The server's MQTT bridge should treat retained `state` as the latest whole
snapshot for a hub, not as a partial update. When it subscribes, the broker
delivers the retained snapshot (if one exists), followed by new snapshots.
The app can mirror that model through the server's app API. Key each device by
the pair (`hubId`, device `id`) so devices remain distinct across hubs;
channel commands are one-based, so channel array index `0` maps to
`channel: 1`.

Track hub connectivity from the separate `availability` topic. A retained
state snapshot can remain in the broker after the hub disconnects; do not
interpret its last `online` values as proof that the hub is currently
connected. `device.online` is the hub's view of the individual device and is
not the same as hub availability. `lastSeenSec` is sampled when the hub
publishes a snapshot, and may be absent.

After publishing a command, correlate the ack by `requestId`. An ack with
`status: "sent"` only confirms that the hub queued/sent the device command;
show the resulting observed value from a later `state` snapshot. For name
commands, `status: "ok"` indicates the hub stored the change; the updated name
will appear in a subsequent snapshot. Do not treat an MQTT publish success by
itself as a device acknowledgement.

The hub has no dedicated per-device detail topic: current device information
is in the single retained `state` array. To subscribe to multiple hubs, the
server can subscribe to exact per-hub topics or use a broker wildcard such as
`iranode/v1/+/state`, subject to its ACLs.

## Local HTTP API

The local-only web interface remains hosted at `http://192.168.4.1`. The
dashboard's device APIs are available only after configuring the hub AP.
They continue to use URL-encoded POST forms, not MQTT or JSON request bodies.
The new online server should use MQTT instead of trying to call these HTTP
routes remotely.

| Method and path | Data / behavior |
|---|---|
| `GET /` | Dashboard HTML when configured; redirects to `/wifi` before configuration. |
| `GET /api/devices` | Device list; online devices include inline detail, offline devices are lightweight. |
| `GET /api/device/detail?id=<hex-id>` | Fetch one device's stored detail. |
| `POST /api/device/relay` | Form fields `id`, `channel`, `value`. |
| `POST /api/device/color` | Form fields `id`, `channel`, `slot` (`1` on / `0` off), `color`. |
| `POST /api/device/name` | Form fields `id`, `name`. |
| `POST /api/device/channel-name` | Form fields `id`, `channel`, `name`. |
| `GET /wifi`, `GET /api/wifi-config`, `POST /api/wifi-config` | Configure the hub's local access point. |
| `GET /network`, `GET /api/network-config`, `POST /api/network-config` | Configure router uplink and MQTT broker locally. |

`GET /api/network-config` reports the uplink SSID, broker host/port/username,
TLS setting, and current Wi-Fi/MQTT connection booleans. It never returns the
router password, broker password, or CA certificate. `POST /api/network-config`
accepts URL-encoded `uplinkSsid`, `uplinkPassword`,
`mqttHost`, `mqttPort`, `mqttUsername`, `mqttPassword`, `mqttTls` (`0` or
`1`), and `mqttCaCert` (PEM). SSID is limited to 32 bytes, router password to
63, broker host to 95, username to 64, broker password to 96, and PEM CA
certificate to 2048 bytes. `mqttPort` must be 1–65535 when a broker host is
set. Empty password/certificate fields preserve saved secrets, and an empty
broker host disables MQTT. A successful save returns `200 text/plain` with
`saved - restarting`.

The local API returns plain-text mutation errors (`400`, `403`, `404`, or
`409`) and does not provide Internet authentication, TLS, or push updates.

## Implementation notes

- MQTT configuration and credentials are stored in LittleFS at
  `/config/network.bin`.
- MQTT dependency: ArduinoMqttClient 0.1.8. JSON encoding/decoding uses
  ArduinoJson 7. Install them with
  `arduino-cli lib install ArduinoMqttClient@0.1.8 ArduinoJson@7.4.3`.
- The hub must have a valid wall-clock time for broker TLS verification; the
  station interface requests NTP time from `pool.ntp.org` or
  `time.cloudflare.com`.
- MQTT commands are capped at 768 bytes. Oversized commands and messages
  delivered marked as retained are not executed; the server must still publish
  with retain disabled as described above.
- AP, router, and broker credentials are excluded from MQTT messages.
