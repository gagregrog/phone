# Phone

ESP32-based controller for an ITT C4 Bell Ringer telephone. Drives the bell ringer motor via an H-bridge and exposes a REST API for remote control over WiFi.

## Hardware

- ESP32 DevKitC
- H-bridge motor driver (IN1, IN2, ENA)
- ITT C4 Bell Ringer telephone
- Momentary button (active LOW with internal pull-up)

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
{"status": "ringing"}
```

### `POST /ring/stop`

Stop all ringing immediately.

```
curl -X POST http://phone.local/ring/stop
```

Response:
```json
{"status": "stopped"}
```

### `POST /ring/pattern`

Start the standard US telephone ring cadence (2 seconds on, 4 seconds off, repeating).

```
curl -X POST http://phone.local/ring/pattern
```

Response:
```json
{"status": "pattern"}
```

### `GET /ring/status`

Check whether the phone is currently ringing.

```
curl http://phone.local/ring/status
```

Response:
```json
{"ringing": true}
```
```json
{"ringing": false}
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
