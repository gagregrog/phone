# Phone

ESP32-based controller for an ITT C4 Bell Ringer telephone. Drives the bell ringer motor via an H-bridge and exposes a REST API for remote control over WiFi.

## Hardware

### Components

- ESP32 DevKitC
- L298N dual H-bridge motor driver module
- ITT C4 Bell Ringer telephone
- Momentary push button (normally open)
- Handset hook switch (normally open)
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

#### Rotary Dial

| Dial Contact | Connection    |
| ------------ | ------------- |
| Off-normal   | ESP32 GPIO 26 |
| Pulse        | ESP32 GPIO 33 |
| Common (GND) | ESP32 GND     |

No external pull-up resistors are needed — the firmware enables the ESP32's internal pull-ups on both pins.

- **Off-normal** contact closes (pulls GPIO 26 LOW) when the dial is in motion and opens when it returns to rest.
- **Pulse** contact pulses LOW once per digit as the dial springs back (10 pulses = 0).

When a digit is dialed it is logged via the standard log output.

#### Hook Switch (Handset)

| Hook Switch Pin | Connection    |
| --------------- | ------------- |
| One side        | ESP32 GPIO 13 |
| Other side      | ESP32 GND     |

No external pull-up resistor is needed — the firmware enables the ESP32's internal pull-up on GPIO 13. The switch is open when the handset is resting in the cradle (on hook) and closes when the handset is lifted (off hook), pulling GPIO 13 LOW.

#### Carbon Microphone

An ITT telephone carbon microphone is conditioned and level-shifted before feeding the ESP32 ADC.

**Bias and preamplifier circuit**

The carbon mic requires a DC bias current to operate. A 9 V supply feeds current through a 330 Ω resistor into the mic, which returns to GND. The junction between the resistor and the mic is the AC signal tap point. A 10 µF electrolytic capacitor (positive leg toward the resistor) AC-couples that tap into pin 3 (non-inverting input) of an NTE823 audio amplifier IC.

| NTE823 Pin | Connection                  |
| ---------- | --------------------------- |
| 2          | GND                         |
| 3          | Mic tap via 10 µF cap (+)   |
| 4          | GND                         |
| 6          | 9 V supply                  |
| 5          | Amplifier output            |

The amplifier output (pin 5) passes through a 220 µF electrolytic capacitor (positive leg toward pin 5) into a 470 Ω resistor in series with a potentiometer that acts as a volume control. The wiper of the potentiometer is the audio output node, which swings approximately ±3 V around the amplifier's bias point.

**Level-shift / bias circuit (amplifier output → ESP32 ADC)**

The ±3 V swing from the volume control is re-centered to the 0–3.3 V range accepted by the ESP32 ADC using a resistive voltage divider with a mid-rail reference:

```
3.3V ──[R1: 10kΩ]──┐
                    ├── ESP32 ADC input
Audio Out ─[R3: 10kΩ]┤
                    ├──[C1: 100nF]── GND
GND ──[R2: 10kΩ]───┘
```

R1 and R2 create a 1.65 V mid-rail reference. R3 mixes the audio signal into that reference, centering the swing around 1.65 V. C1 (100 nF) bypasses high-frequency noise on the ADC node to GND. The resulting signal stays within the 0–3.3 V ADC input range.

**Full signal chain**

```
9V ──[330Ω]──┬──── Carbon Mic ──── GND
             │
         [10µF +]              ← AC coupling into amp
             │
         NTE823 pin 3 (input)
         NTE823 pin 6 ──── 9V
         NTE823 pin 2 ──── GND
         NTE823 pin 4 ──── GND
         NTE823 pin 5 (output)
             │
         [220µF +]             ← AC coupling out of amp
             │
          [470Ω]
             │ (wiper)
           [POT]
             │ (left leg)
         [R3: 10kΩ]
             │
             ├──────────────────── ESP32 ADC
             ├──[C1: 100nF]──── GND
             │
   ┌─────────┴─────────┐
[R1: 10kΩ]         [R2: 10kΩ]
   │                    │
  3.3V                 GND
```

#### Carbon Microphone ADC

| Signal           | ESP32 Pin       | Notes                                              |
| ---------------- | --------------- | -------------------------------------------------- |
| Mic bias output  | GPIO 34 (ADC1_CH6) | Bias-conditioned 0–2.2 V; `ADC_ATTEN_DB_6` attenuation |

GPIO 34 is input-only and has no internal pull-up. The level-shift circuit described above ensures the signal stays within the 0–2.2 V range suited to `ADC_ATTEN_DB_6`. Do not use `ADC_ATTEN_DB_11` (0–3.3 V) — the top third of the ADC range would be wasted.

The firmware samples at 8 kHz using I2S ADC DMA mode (`I2S_MODE_ADC_BUILT_IN`) and streams 64-sample packets of 16-bit signed PCM over WebSocket to browser clients. Audio is only sampled when at least one WebSocket client is connected.

#### Pin Reference

All pin assignments are defined in `include/pins.h` and can be changed there if needed.

## Build & Upload

Flash over USB (first time or after hardware changes):

```
pio run -e esp32dev -t upload
```

Flash over WiFi (OTA):

```
pio run -e esp32ota -t upload
```

`esp32ota` is the default environment, so the VSCode PlatformIO upload button will use OTA automatically.

### .env Configuration

Build and upload settings are read from a `.env` file in the project root (gitignored). Create one with the following keys:

```
OTA_PASSWORD=yourpassword        # compiled into firmware; required for OTA uploads
TZ_STRING=EST5EDT,M3.2.0,M11.1.0 # POSIX timezone string; compiled into firmware
IP=192.168.x.x                  # device IP for OTA uploads
PORT=/dev/tty.usbserial-xxxx    # serial port for USB upload and monitoring
```

`TZ_STRING` is a POSIX timezone string that controls both the UTC offset and DST rules. Common examples:

| Timezone       | `TZ_STRING`                  |
| -------------- | ---------------------------- |
| US Eastern     | `EST5EDT,M3.2.0,M11.1.0`     |
| US Central     | `CST6CDT,M3.2.0,M11.1.0`     |
| US Mountain    | `MST7MDT,M3.2.0,M11.1.0`     |
| US Pacific     | `PST8PDT,M3.2.0,M11.1.0`     |
| UK             | `GMT0BST,M3.5.0/1,M10.5.0`   |
| Central Europe | `CET-1CEST,M3.5.0,M10.5.0/3` |
| UTC            | `UTC0`                       |

If `TZ_STRING` is omitted from `.env`, the firmware defaults to `UTC0`.

To switch between serial ports, comment out the one you don't want — commented lines (prefixed with `#`) are ignored:

```
#PORT=/dev/tty.usbserial-1450
PORT=/dev/tty.usbserial-110
```

Generate a random OTA password:

```
python3 -c "import secrets; print('OTA_PASSWORD=' + secrets.token_hex(12))"
```

If `.env` is missing, the build will warn and the firmware will use an empty OTA password.

> **Note:** A VPN on the host machine will typically block OTA uploads. Disable it before uploading.

## Partition Table

The project uses a custom partition table (`partitions.csv`) that removes the unused SPIFFS partition and extends each OTA app slot from **1.25 MB → 1.875 MB**, giving substantially more headroom for firmware growth.

| Partition | Offset     | Size       | Notes                          |
| --------- | ---------- | ---------- | ------------------------------ |
| nvs       | `0x009000` | 20 KB      | Key/value storage (Preferences)|
| otadata   | `0x00E000` | 8 KB       | Tracks which OTA slot is active|
| app0      | `0x010000` | 1.875 MB   | OTA slot 0                     |
| app1      | `0x200000` | 1.875 MB   | OTA slot 1                     |

### Web UI build step

The web UI source lives in `src/web/web_ui.html`. Each device build automatically gzip-compresses it and writes `src/web/web_ui_html.h` (a byte array included by `WebUI.cpp`). The generated header is gitignored — edit `web_ui.html` directly, never the header.

The build will print the compression result, e.g.:

```
compress_html: 27134 -> 7823 bytes (71% reduction)
```

This step does not run for the `native` test environment.

### Applying the partition table

The partition table lives outside the app partitions and **cannot be updated over OTA** — it requires a one-time serial flash. After that, OTA works normally.

Connect the ESP32 via USB and run:

```
pio run -e esp32dev -t upload
```

This flashes the bootloader, partition table, and firmware. Your NVS data (WiFi credentials, saved alarms) is **preserved** — the NVS partition sits at the same offset and size as the factory default table and is not touched during a normal upload.

### Reverting to the default partition table

Remove `board_build.partitions = partitions.csv` from `platformio.ini` and delete `partitions.csv`, then flash via serial:

```
pio run -e esp32dev -t upload
```

NVS data is still preserved when reverting.

## Logs & Debugging

### Log Categories

Each log entry is tagged with a category that identifies the part of the system it came from. Categories appear as the prefix in all log outputs and are color-coded in the web UI:

| Category | Color | Covers |
| ------------ | ------ | ------- |
| `[HARDWARE]` | cyan | Raw hardware events: handset lifted/returned, dial pulses, ring motor start/stop |
| `[SCHEDULER]` | orange | Timed events: timer expiry, alarm firing, hourly clock chime |
| `[PHONE]` | green | Call state machine: ring suppressed/rejected/answered/stopped, handset off-hook/on-hook, dialing |
| `[API]` | blue | HTTP requests: method, path, result |

Startup and WiFi messages are uncategorized and appear as `[INFO]`, `[WARN]`, or `[ERROR]`.

Example log sequence for an inbound ring that is answered and then ended:

```
[22:27:41] [HARDWARE] Ring started: us
[22:27:44] [HARDWARE] Handset: lifted
[22:27:44] [PHONE]    Call answered
[22:27:49] [HARDWARE] Handset: returned
[22:27:49] [PHONE]    Handset on hook [was IN_CALL]
```

### Log destinations

Log output is written to two destinations simultaneously:

- **USB serial** — always available; useful during initial setup and boot
- **Web UI** — the Logs panel in the dashboard streams live log output over WebSocket, with timestamps (once NTP is synced) and color-coded severity. The device buffers the last 100 log entries, so history is visible immediately when you open the page.

Boot messages before WiFi is up are only visible on USB serial.

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

## Phone Behavior

The firmware enforces standard hook-switch rules:

- **Ring suppressed when off hook** — ringing will not start (via button, API, alarm, timer, or clock) if the handset is already lifted. Any `POST /ring/<pattern>` call returns `409 {"busy": true}` in this case.
- **Pickup stops an active ring** — lifting the handset while the phone is ringing stops the bell immediately.
- **Hang up clears all state** — returning the handset to the cradle ends any active call or dialing session and returns the phone to idle.

## Button

The physical button acts as a toggle:

- Press once to start the US ring pattern (2s on, 4s off)
- Press again to stop ringing
- Stops ringing regardless of whether it was started by the button or the API
- Does nothing if the handset is off hook

## Ringing Patterns

Multiple country-specific ringing cadences are available:

| Pattern | Name    | Cadence                                                   |
| ------- | ------- | --------------------------------------------------------- |
| US      | `us`    | 2s on, 4s off                                             |
| UK      | `uk`    | 0.4s on, 0.2s off, 0.4s on, 2s off (double ring)          |
| Germany | `de`    | 1s on, 4s off                                             |
| France  | `fr`    | 1.5s on, 3.5s off                                         |
| Japan   | `jp`    | 1s on, 2s off                                             |
| Italy   | `it`    | 1s on, 1s off, 1s on, 3s off (double ring)                |
| Sweden  | `se`    | 1s on, 5s off                                             |
| Chirp   | `chirp` | 0.15s on, 0.1s off, 0.15s on, 0.6s off (two quick bursts) |
| Chime   | `chime` | 0.4s on, 0.4s off (used by hourly clock chime)            |

## Web UI

A browser-based dashboard is served at `http://phone.local/` (port 80). The source lives in `src/web/web_ui.html` and is gzip-compressed automatically at build time. It provides a live view of all device state and controls for every feature:

- **Ringer** — see ring status, trigger any pattern with one click, stop ringing
- **Timers** — view active timers with countdowns, add new timers, cancel individually or all at once
- **Alarms** — view scheduled alarms, add new alarms, edit existing alarms in-place, delete individually or all
- **Hourly chime** — see enabled state and mode, toggle on/off, switch between `n_chimes` and `single` mode
- **Handset** — live on-hook / off-hook status displayed as an animated SVG of the rotary phone; updates in real time via WebSocket
- **Microphone** — live oscilloscope waveform fed by the carbon mic via I2S ADC DMA at 8 kHz; active only while the browser tab is open
- **Logs** — live streaming log console with color-coded severity (info/warn/error) and timestamps. Displays the last 100 log entries buffered on the device, so recent history is visible immediately on page load without needing to wait for new messages. A **Clear** button wipes the display.

The dashboard connects via WebSocket (`ws://phone.local/ws`) and updates in real time whenever state changes — including hardware-triggered events like timer expiry, alarm fires, button presses, and hourly chimes. A **Live / Offline** badge in the header shows the connection status; the page reconnects automatically if the connection drops.

## REST API

The API runs on port 80. All endpoints are non-blocking.

### Ring

#### `GET /ring/status`

Check whether the phone is currently ringing.

```
curl http://phone.local/ring/status
```

Response:

```json
{ "ringing": false }
```

#### `POST /ring/<pattern>[/<count>]`

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

If the handset is off hook, an active ring is already in progress, or the phone is in a call, the request is rejected with `409 Conflict`:

```json
{ "busy": true }
```

#### `POST /ring/stop`

Stop all ringing immediately.

```
curl -X POST http://phone.local/ring/stop
```

Response:

```json
{ "status": "stopped" }
```

#### `GET /ring/patterns`

List all available ringing pattern names.

```
curl http://phone.local/ring/patterns
```

Response:

```json
["us", "uk", "de", "fr", "jp", "it", "se", "chirp"]
```

### Timer

#### `GET /timer/status`

List all active timers.

```
curl http://phone.local/timer/status
```

Response:

```json
[
  { "id": 1, "remaining": "2m58s", "total": "5m", "pattern": "chirp" },
  { "id": 2, "remaining": "10m", "total": "1h", "pattern": "us" }
]
```

Returns an empty array if no timers are active.

#### `POST /timer/<duration>[/<pattern>]`

Start a countdown timer. When it expires, the phone rings automatically. Defaults to the `chirp` pattern (1 cycle) if no pattern is specified. Multiple timers can be active simultaneously.

Duration supports `h` (hours), `m` (minutes), and `s` (seconds) suffixes, which can be combined in order. At least one unit suffix is required. Valid range: 1 second to 24 hours.

When combining units, sub-units are capped at 59 (e.g. `1h90m` is invalid — use `2h30m`).

| Example | Duration            |
| ------- | ------------------- |
| `20m`   | 20 minutes          |
| `90s`   | 90 seconds          |
| `1h30m` | 1 hour 30 minutes   |
| `1m30s` | 1 minute 30 seconds |
| `2h`    | 2 hours             |

```
curl -X POST http://phone.local/timer/20m
curl -X POST http://phone.local/timer/1h30m
curl -X POST http://phone.local/timer/90s/us
```

Response:

```json
{ "status": "started", "id": 1, "duration": "5m", "pattern": "chirp" }
```

The returned `id` can be used to cancel this specific timer later. When a timer fires it overrides any active ring.

#### `POST /timer/cancel/<id>`

Cancel a specific timer by its ID without triggering the ring. The ID is returned when the timer is created.

```
curl -X POST http://phone.local/timer/cancel/1
```

Response:

```json
{ "status": "cancelled", "id": 1 }
```

If the ID is not found:

```json
{ "error": "not found" }
```

#### `POST /timer/cancel`

Cancel all active timers without triggering any rings.

```
curl -X POST http://phone.local/timer/cancel
```

Response:

```json
{ "status": "cleared", "count": 2 }
```

### Alarm

#### `GET /alarm`

List all scheduled alarms.

```
curl http://phone.local/alarm
```

Response:

```json
[
  {
    "id": 1,
    "time": "08:30",
    "pattern": "us",
    "rings": 3,
    "repeat": true,
    "skipWeekends": false,
    "enabled": true
  },
  {
    "id": 2,
    "time": "17:00",
    "pattern": "chirp",
    "rings": 1,
    "repeat": false,
    "skipWeekends": false,
    "enabled": true
  }
]
```

#### `POST /alarm`

Schedule a new alarm. Fires at the given local time using the timezone configured in `TZ_STRING`.

```
curl -X POST http://phone.local/alarm \
  -H 'Content-Type: application/json' \
  -d '{"hour": 8, "minute": 30, "pattern": "us", "rings": 3, "repeat": true, "skipWeekends": false}'
```

| Field          | Type   | Description                                 |
| -------------- | ------ | ------------------------------------------- |
| `hour`         | int    | 0–23 local time                             |
| `minute`       | int    | 0–59                                        |
| `pattern`      | string | Ring pattern name (see table above)         |
| `rings`        | int    | Number of ring cycles (0 = infinite)        |
| `repeat`       | bool   | `true` to repeat daily; `false` for one-off |
| `skipWeekends` | bool   | Skip Saturday and Sunday when `true`        |

One-off alarms (`repeat: false`) require NTP to be synced and the target time to be in the future. Returns `503` if NTP is not yet synced, `400` if the time has already passed today. Repeating alarms are persisted to flash and restored after reboot; one-off alarms live in memory only.

Response:

```json
{
  "id": 1,
  "time": "08:30",
  "pattern": "us",
  "rings": 3,
  "repeat": true,
  "skipWeekends": false,
  "enabled": true
}
```

#### `PUT /alarm/{id}`

Update an existing alarm (re-enables it if previously disabled).

```
curl -X PUT http://phone.local/alarm/1 \
  -H 'Content-Type: application/json' \
  -d '{"hour": 9, "minute": 0, "pattern": "uk", "rings": 2, "repeat": true, "skipWeekends": true}'
```

Returns `404` if the ID is not found.

#### `DELETE /alarm/{id}`

Delete a specific alarm.

```
curl -X DELETE http://phone.local/alarm/1
```

Response:

```json
{ "status": "deleted", "id": 1 }
```

#### `DELETE /alarm`

Delete all alarms.

```
curl -X DELETE http://phone.local/alarm
```

Response:

```json
{ "status": "cleared" }
```

### Clock

The hourly chime strikes at the top of each hour using the `chime` pattern (400ms on, 400ms off). It will not interrupt an active alarm or timer ring. Both the enabled state and mode persist across reboots.

Two modes are available:

| Mode       | Behavior                                                                  |
| ---------- | ------------------------------------------------------------------------- |
| `n_chimes` | Rings N times matching the 12-hour clock (1 at 1:00, 12 at noon/midnight) |
| `single`   | Rings once at the top of every hour                                       |

#### `GET /clock`

Return the current enabled state and chime mode.

```
curl http://phone.local/clock
```

Response:

```json
{ "enabled": false, "mode": "n_chimes" }
```

#### `POST /clock/toggle`

Toggle the hourly chime on or off. Returns the new state.

```
curl -X POST http://phone.local/clock/toggle
```

Response:

```json
{ "enabled": false, "mode": "n_chimes" }
```

#### `POST /clock/mode/toggle`

Switch between `n_chimes` and `single` mode. Returns the new state.

```
curl -X POST http://phone.local/clock/mode/toggle
```

Response:

```json
{ "enabled": true, "mode": "single" }
```

### Handset

#### `GET /handset/status`

Return the current hook switch state.

```
curl http://phone.local/handset/status
```

Response:

```json
{ "offHook": false }
```

`offHook` is `true` when the handset is lifted (off hook) and `false` when it is resting in the cradle (on hook). State changes are also broadcast over WebSocket as `handset/up` and `handset/down` events.

### Device

#### `GET /ip`

Returns the device's IP address as plain text. Useful for adding a static entry to `/etc/hosts` to avoid slow mDNS resolution.

```
curl http://phone.local/ip
```

Response:

```
192.168.1.100
```
