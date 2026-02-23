# Phone Project

ESP32 DevKitC project using PlatformIO with Arduino framework.

## Build & Upload

The user will run all PlatformIO commands (`pio run`, `pio run -t upload`, etc.) themselves. Do not run `pio` commands.

## Code Style

- Keep `main.cpp` minimal — implement all non-trivial logic in separate modules (header in `include/`, implementation in `src/`)
- Avoid magic numbers for pin assignments — use defines in `include/pins.h`

## Project Structure

- `include/` — Header files (`.h`)
- `src/` — Implementation files (`.cpp`)
- `include/pins.h` — All GPIO pin definitions
- `platformio.ini` — PlatformIO configuration
