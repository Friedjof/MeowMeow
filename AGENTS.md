# Agent Instructions - ESP32 PlatformIO Template (with Web UI)

This file contains instructions for AI agents working on this project.

## Project Overview

This template is a lean PlatformIO project for ESP32 devices.

- PlatformIO: ESP32 firmware development (Arduino framework)
- Build tools: Makefile and `tools/setup.sh`
- Base firmware: open AP with web UI for LED on/off in `src/main.cpp`

## Project Structure

```
.
├── boards/                 # Partition schemes
├── data-template/          # Filesystem template (optional)
├── include/                # C/C++ header files
├── lib/                    # Custom libraries
├── src/                    # ESP32 firmware (C++)
│   └── main.cpp           # Main firmware file (setup/loop)
├── test/                   # Unit tests
├── tools/
│   └── setup.sh           # Initialize project environment
├── Makefile               # Build helpers
└── platformio.ini         # PlatformIO configuration
```

## Key Workflows

### 1. Repository setup

```bash
./tools/setup.sh
```

### 2. Build firmware

```bash
pio run -e esp32
pio run -e esp32c3
pio run -e esp32s3
pio run -e esp32c6
```

### 3. Flash firmware

```bash
pio run -e esp32 -t upload
pio device monitor
```

### 4. Filesystem (optional)

```bash
make deploy-fs
```

## Important Files

- `src/main.cpp` - Setup/loop, SoftAP + web UI for LED on/off
- `platformio.ini` - Build targets and dependencies
- `data-template/config.json` - Sample configuration

## Configure LED and AP (`src/main.cpp`)

```cpp
const int LED_PIN = LED_BUILTIN;
const uint8_t LED_ON_LEVEL = HIGH;
const char* AP_SSID = "MeowMeow";
```

## Best Practices

1. Prefer non-blocking code (millis instead of delay)
2. Use serial logging for debugging
3. Keep an eye on free heap (`ESP.getFreeHeap()`)

## Git Workflow

**Commit:**
- `src/`, `include/`, `lib/` - Firmware files
- `tools/` - Scripts
- `data-template/` - Template files
- `platformio.ini`, README files

**Do not commit:**
- `data/` (generated from data-template)
- `.pio/`, `.pio-home/` (PlatformIO cache)

See `.gitignore` for the full list.

## Debugging

```bash
pio device monitor
```

Baud rate: 115200 (see `platformio.ini`)

## Support & Resources

- PlatformIO Docs: https://docs.platformio.org/
- ESP32 Arduino Core: https://docs.espressif.com/projects/arduino-esp32/
