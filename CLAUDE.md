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

## Project Structure

- `include/` — Header files (`.h`)
- `src/` — Implementation files (`.cpp`)
- `include/pins.h` — All GPIO pin definitions
- `platformio.ini` — PlatformIO configuration

## Testing

After adding a new feature or modifying existing logic, run `pio test -e native` to check for regressions.

Tests run natively on the host machine via `pio test -e native`. Each test lives in its own subdirectory under `test/` (e.g. `test/test_timer/test_timer.cpp`). A minimal Arduino mock at `test/mock/Arduino.h` provides a controllable `millis()` and GPIO stubs. When adding a new testable module:
- Add its `.cpp` to the native `build_src_filter` in `platformio.ini`
- Create a `test/test_<name>/test_<name>.cpp` with a `main()` using Unity
