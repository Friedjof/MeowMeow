# ESP32 PlatformIO Template

A lean template project for ESP32 development with PlatformIO.

> For AI agents: see [AGENTS.md](AGENTS.md) for instructions on working with this project.

## Project Structure

```
.
├── boards/                 # Partition schemes
├── data-template/          # Filesystem template (optional)
├── include/                # Header files
├── lib/                    # Project libraries
├── src/                    # ESP32 firmware source
├── test/                   # Unit tests
├── tools/                  # Build and setup tools
│   └── setup.sh            # Repository setup
├── Makefile                # Build helpers
└── platformio.ini          # PlatformIO configuration
```

## Features

- PlatformIO: multi-target ESP32 support (ESP32, C3, S3, C6)
- Build automation: Makefile for build, flash, monitor
- Base firmware: open AP with a cat-themed web UI to toggle the LED (config in `src/main.cpp`)
- Optional filesystem: `data-template/` for configuration files

## Prerequisites

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
- Git (optional)

## Quick Start

### 1. Repository setup

```bash
./tools/setup.sh
```

### 2. Build firmware

```bash
pio run -e esp32
```

Or for another target:

```bash
pio run -e esp32c3   # ESP32-C3
pio run -e esp32s3   # ESP32-S3
pio run -e esp32c6   # ESP32-C6
```

### 3. Upload firmware

```bash
pio run -e esp32 --target upload
```

### 4. Serial monitor (optional)

```bash
pio device monitor
```

## PlatformIO Commands

| Command | Description |
|---------|-------------|
| `pio run` | Build firmware |
| `pio run -e <env>` | Build for a specific environment |
| `pio run -t upload` | Upload firmware |
| `pio run -t uploadfs` | Upload filesystem |
| `pio run -t monitor` | Open serial console |
| `pio device monitor` | Start monitor |
| `pio test` | Run unit tests |

## Customization

### Add dependencies

Edit `platformio.ini`:

```ini
[env]
lib_deps =
    ; Add libraries here...
```

### Configure LED and AP

Edit `src/main.cpp`:

```cpp
const int LED_PIN = LED_BUILTIN;
const uint8_t LED_ON_LEVEL = HIGH;
const char* AP_SSID = "MeowMeow";
```

## Partition Scheme

This template uses `min_spiffs.csv` for OTA updates and a small filesystem.

For custom partitions, create `boards/min_spiffs.csv`:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x1E0000
app1,     app,  ota_1,   0x1F0000,0x1E0000
spiffs,   data, spiffs,  0x3D0000,0x30000
```
