# Phone Project

ESP32 DevKitC project using PlatformIO with Arduino framework.

## Build & Upload

The user will run all PlatformIO commands (`pio run`, `pio run -t upload`, etc.) themselves. Do not run `pio` commands.

## Code Style

- Keep `main.cpp` minimal — implement all non-trivial logic in separate modules (header in `include/`, implementation in `src/`)
- Avoid magic numbers for pin assignments — use defines in `include/pins.h`
- Where possible, keep core logic free of Arduino dependencies so it can be unit tested natively:
  - Use `<stdint.h>` instead of `<Arduino.h>` when only basic types are needed
  - Extract pure logic (parsing, state machines, calculations) into standalone modules
  - Modules that use `millis()` or GPIO are still testable via the mock at `test/mock/Arduino.h`

## Dependencies

When adding a new library, pin it to the exact version being implemented (e.g. `bblanchon/ArduinoJson@7.4.2`). Never use unpinned or range-based version specifiers. This prevents unexpected breaking changes from upstream updates.

## Project Structure

- `include/` — Header files (`.h`)
- `src/` — Implementation files (`.cpp`)
- `include/pins.h` — All GPIO pin definitions
- `platformio.ini` — PlatformIO configuration

## Logging

Use `logger` (not `Serial` directly) for all log output after WiFi is up. Never log sensitive values — this includes passwords, tokens, or raw credential data. The Telnet server transmits log output in plaintext with no authentication.

## Documentation

Update `README.md` whenever making changes that affect how the project is built, configured, or used — including build flags, `.env` keys, API changes, hardware wiring, or new features.

## Testing

After adding a new feature or modifying existing logic, run `pio test -e native` to check for regressions.

Tests run natively on the host machine via `pio test -e native`. Each test lives in its own subdirectory under `test/` (e.g. `test/test_timer/test_timer.cpp`). A minimal Arduino mock at `test/mock/Arduino.h` provides a controllable `millis()` and GPIO stubs. When adding a new testable module:
- Add its `.cpp` to the native `build_src_filter` in `platformio.ini`
- Create a `test/test_<name>/test_<name>.cpp` with a `main()` using Unity
- Mock headers live in `test/mock/` (e.g. `Arduino.h`). Source files that support test mocks (e.g. `mock_millis.cpp`) go in `src/test/` — this directory is included in the native build but excluded from the ESP32 build
