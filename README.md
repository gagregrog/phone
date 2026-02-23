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

- Press once to start the standard US ring pattern (2s on, 4s off)
- Press again to stop ringing
- Stops ringing regardless of whether it was started by the button or the API

## REST API

The API runs on port 80. All endpoints are non-blocking.

### `POST /ring/start`

Start continuous ringing (motor oscillates indefinitely).

```
curl -X POST http://phone.local/ring/start
```

Response:

```json
{ "status": "ringing" }
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

### `POST /ring/pattern`

Start the standard US telephone ring cadence (2 seconds on, 4 seconds off, repeating).

```
curl -X POST http://phone.local/ring/pattern
```

Response:

```json
{ "status": "pattern" }
```

### `GET /ring/status`

Check whether the phone is currently ringing.

```
curl http://phone.local/ring/status
```

Response:

```json
{ "ringing": true }
```

```json
{ "ringing": false }
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
