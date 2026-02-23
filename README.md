# Phone

ESP32-based controller for an ITT C4 Bell Ringer telephone. Drives the bell ringer motor via an H-bridge and exposes a REST API for remote control over WiFi.

## Hardware

### Components

- ESP32 DevKitC
- L298N dual H-bridge motor driver module
- ITT C4 Bell Ringer telephone
- Momentary push button (normally open)
- External power supply for the L298N (voltage depends on your bell ringer motor)

### Wiring

#### L298N Motor Driver

| L298N Pin | Connection              |
| --------- | ----------------------- |
| IN1       | ESP32 GPIO 25           |
| IN2       | ESP32 GPIO 32           |
| ENA       | ESP32 GPIO 27           |
| OUT1      | Bell ringer motor +     |
| OUT2      | Bell ringer motor -     |
| GND       | ESP32 GND               |
| VS        | External power supply + |
| GND       | External power supply - |

Remove the ENA jumper on the L298N if present — the ESP32 controls ENA directly via GPIO 27.

#### ITT C4 Bell Ringer

The C4 ringer comes in 2-wire and 4-wire variants. The 4-wire version has two separate coil windings. This project only uses one winding — connect one pair of wires to OUT1/OUT2 on the L298N and leave the other pair disconnected.

**Identifying the windings with a multimeter:**

1. Set your multimeter to resistance/continuity mode.
2. Test all six possible wire combinations (pick any two of the four wires).
3. Two pairs will show a resistance (on mine one was 970 ohms and the other 2.4 k ohms) — these are the two windings, one pair each.
4. The remaining four combinations will show open circuit (OL / no continuity) — those wires belong to different windings.

Pick either winding and connect that pair to OUT1 and OUT2. Polarity doesn't matter since the H-bridge alternates direction. The unused winding can be left disconnected — it won't affect operation.

#### Button

| Button Pin | Connection    |
| ---------- | ------------- |
| One side   | ESP32 GPIO 14 |
| Other side | ESP32 GND     |

No external pull-up resistor is needed — the firmware enables the ESP32's internal pull-up on GPIO 14.

#### Pin Reference

All pin assignments are defined in `include/pins.h` and can be changed there if needed.

## Tests

Unit tests run natively on your computer (no ESP32 needed):

```
pio test -e native
```

## WiFi Setup

On first boot (or when saved credentials are unavailable), the ESP32 creates a WiFi access point called **PhoneSetup**. Connect to it and a captive portal will appear where you can enter your WiFi credentials. They are saved to flash and used automatically on subsequent boots.

The device registers itself via mDNS as `phone.local`. If mDNS resolution is slow on your network, you can add a static entry to your hosts file instead:

1. Get the device's IP: `curl http://phone.local/ip`
2. Add it to `/etc/hosts`:
   ```
   192.168.x.x    phone.local
   ```

## Button

The physical button acts as a toggle:

- Press once to start the US ring pattern (2s on, 4s off)
- Press again to stop ringing
- Stops ringing regardless of whether it was started by the button or the API

## Ringing Patterns

Multiple country-specific ringing cadences are available:

| Pattern | Name | Cadence |
| ------- | ---- | ------- |
| US | `us` | 2s on, 4s off |
| UK | `uk` | 0.4s on, 0.2s off, 0.4s on, 2s off (double ring) |
| Germany | `de` | 1s on, 4s off |
| France | `fr` | 1.5s on, 3.5s off |
| Japan | `jp` | 1s on, 2s off |
| Italy | `it` | 1s on, 1s off, 1s on, 3s off (double ring) |
| Sweden | `se` | 1s on, 5s off |
| Chirp | `chirp` | 0.15s on, 0.1s off, 0.15s on, 0.6s off (two quick bursts) |

## REST API

The API runs on port 80. All endpoints are non-blocking.

### `POST /ring/<pattern>[/<count>]`

Start ringing with a specific country pattern. Replace `<pattern>` with a pattern name from the table above (e.g., `us`, `uk`, `de`).

Optionally append `/<count>` to ring for a specific number of cycles, then stop automatically. A cycle is one full pass through the pattern's phase array. Omit the count to ring indefinitely.

```
curl -X POST http://phone.local/ring/us
curl -X POST http://phone.local/ring/uk/5
```

Response (infinite):

```json
{ "status": "us" }
```

Response (5 cycles):

```json
{ "status": "uk", "cycles": 5 }
```

### `POST /ring/stop`

Stop all ringing immediately.

```
curl -X POST http://phone.local/ring/stop
```

Response:

```json
{ "status": "stopped" }
```

### `GET /ring/status`

Check whether the phone is currently ringing and the state of any active timer.

```
curl http://phone.local/ring/status
```

Response (with active timer):

```json
{ "ringing": false, "timer": { "active": true, "remainingSec": 178, "totalSec": 300, "pattern": "chirp" } }
```

Response (no active timer):

```json
{ "ringing": false, "timer": { "active": false } }
```

### `GET /ring/patterns`

List all available ringing pattern names.

```
curl http://phone.local/ring/patterns
```

Response:

```json
["us","uk","de","fr","jp","it","se","chirp"]
```

### `POST /timer/<duration>[/<pattern>]`

Start a countdown timer. When it expires, the phone rings automatically. Defaults to the `chirp` pattern (3 cycles) if no pattern is specified.

Duration supports `h` (hours), `m` (minutes), and `s` (seconds) suffixes, which can be combined in order. At least one unit suffix is required. Valid range: 1 second to 24 hours.

When combining units, sub-units are capped at 59 (e.g. `1h90m` is invalid — use `2h30m`).

| Example | Duration |
| ------- | -------- |
| `20m` | 20 minutes |
| `90s` | 90 seconds |
| `1h30m` | 1 hour 30 minutes |
| `1m30s` | 1 minute 30 seconds |
| `2h` | 2 hours |

```
curl -X POST http://phone.local/timer/20m
curl -X POST http://phone.local/timer/1h30m
curl -X POST http://phone.local/timer/90s/us
```

Response:

```json
{ "status": "started", "totalSec": 300, "pattern": "chirp" }
```

Starting a new timer replaces any existing one. When the timer fires, it overrides any active ring.

### `POST /timer/cancel`

Cancel an active timer without triggering the ring.

```
curl -X POST http://phone.local/timer/cancel
```

Response:

```json
{ "status": "cancelled" }
```

If no timer is active:

```json
{ "status": "no_timer" }
```

### `GET /ip`

Returns the device's IP address as plain text. Useful for adding a static entry to `/etc/hosts` to avoid slow mDNS resolution.

```
curl http://phone.local/ip
```

Response:

```
192.168.1.100
```
