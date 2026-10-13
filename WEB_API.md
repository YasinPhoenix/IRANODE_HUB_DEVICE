# Iranode Hub API

## MQTT

Hub IDs are eight lowercase hexadecimal characters from the lower 32 bits of
the ESP32 eFuse MAC. The ID appears in topics, but not in the state payload.

```text
iranode/v1/{hubId}/availability
iranode/v1/{hubId}/state
iranode/v1/{hubId}/command
iranode/v1/{hubId}/ack
```

All topics use QoS 1. `availability` and `state` are retained; `command` and
`ack` are not. The hub publishes `availability`, `state`, and `ack`, and
subscribes only to its own `command` topic.

Commands **must** be published without retain. Retained commands can run old
actions after reconnect. The hub rejects retained deliveries when the broker
preserves the flag, but this is only a safeguard; the server/broker must
prevent retained commands. The hub does not deduplicate `requestId`s, so
retries can produce duplicate acknowledgements and actions.

### Availability

On connect, publish the retained message:

```json
{"online":true}
```

The QoS 1 retained Last Will is:

```json
{"online":false}
```

The hub also publishes retained offline availability before a graceful
Disconnect when possible.

### State

`state` is a retained JSON array containing every known device, including
offline devices. It is published on MQTT connect and every five seconds. Use
`availability` to determine hub connectivity; the retained state may remain
after disconnect. `device.online` describes the device as seen by the hub and
is not hub availability.

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
  {"id": "5e6f7081", "online": false, "lastSeenSec": 48}
]
```

`lastSeenSec` is optional and is measured since the hub booted. It is omitted
until the device has been seen during that boot. Devices without saved detail
contain only availability metadata. A device is offline after more than 15
seconds without a packet.

| Field | Description |
|---|---|
| `id` | Eight-character lowercase hexadecimal device ID. |
| `online` | Whether the hub recently heard from the device. |
| `lastSeenSec` | Optional seconds since the last packet. |
| `name` | Hub-side name; empty means unset. |
| `type` | `1` wall switch, `0` unknown. |
| `fw` | Firmware as `"major.minor"`. |
| `channels` | Reported channel count. |
| `switch` | Wall-switch details with a `channels` array. |

Each channel has `relay` (boolean), `colorOn`, `colorOff` (integers), and
`name` (string). Colors are `0` off, `1` red, `2` green, `3` blue, `4` yellow,
`5` cyan, `6` magenta, and `7` white.

Array order is not an identifier. Key devices by `id` and use one-based channel
numbers (`array index + 1`).

### Commands

Publish JSON to `iranode/v1/{hubId}/command` with QoS 1 and retain disabled.
Every command needs a unique `requestId`, a `command`, and a target `id`.

| Command | Required fields | Example |
|---|---|---|
| `relay` | `id`, `channel`, `value` | `{"requestId":"req-1","command":"relay","id":"1a2b3c4d","channel":1,"value":true}` |
| `color` | `id`, `channel`, `slot`, `color` | `{"requestId":"req-2","command":"color","id":"1a2b3c4d","channel":1,"slot":"on","color":3}` |
| `name` | `id`, `name` | `{"requestId":"req-3","command":"name","id":"1a2b3c4d","name":"Living room"}` |
| `channel_name` | `id`, `channel`, `name` | `{"requestId":"req-4","command":"channel_name","id":"1a2b3c4d","channel":1,"name":"Main"}` |
| `get_state` | `id`, `channel` | `{"requestId":"req-5","command":"get_state","id":"1a2b3c4d","channel":1}` |

`relay.channel` and `color.channel` must exist in the saved device detail.
`relay.value` accepts a boolean or `0`/`1`. `color.slot` is `"on"` or
`"off"`, and `color` is `0`–`7`.

Device names are limited to 23 UTF-8 bytes; channel names to 15 bytes. Empty
names clear them. `channel_name` accepts slots `1`–`4`, regardless of the
reported channel count. Names are stored on the hub, not sent to the switch.

`get_state` requests a fresh physical-device report. Its acknowledgement only
confirms that the request was sent; the result appears in a later `state`
snapshot.

### Acknowledgements

The hub publishes one ack per valid command with a non-empty `requestId` and
`command`:

```json
{"requestId":"req-1","command":"relay","ok":true,"status":"sent"}
```

`status: "sent"` means a relay, color, or state request was queued/sent to an
online device, not that the physical change completed. Use a later state
snapshot for observed values. Successful name changes use `status: "ok"`.

| Status | Meaning |
|---|---|
| `invalid_device_id` | ID is not exactly eight hexadecimal characters. |
| `unknown_device` | Device is not registered on the hub. |
| `invalid_arguments` | Missing or invalid command field; also used for an out-of-range `channel_name`. |
| `invalid_channel` | Channel is outside `1`–`4`, or detail is unavailable for validation. |
| `invalid_name` | Name is not a string or exceeds its byte limit. |
| `invalid_command` | Command is not `relay`, `color`, `get_state`, `name`, or `channel_name`. |
| `device_not_online` | A physical-device request could not be sent. |
| `retained_command_rejected` | The command arrived with retain set. |
