# Quick Start Guide

Quick guide to test the ESP32 template.

## Prerequisites

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) installed
- ESP32 board with USB cable

## A working system in 5 minutes

### 1. Repository setup

```bash
./tools/setup.sh
```

### 2. Build firmware

```bash
pio run -e esp32
```

### 3. Flash firmware

```bash
pio run -e esp32 -t upload
```

Or for another board:
```bash
pio run -e esp32c3 -t upload   # ESP32-C3
pio run -e esp32s3 -t upload   # ESP32-S3
```

**Note:** You may need to hold the BOOT button during upload.

### 4. Serial monitor (optional)

```bash
pio device monitor
```

You should see:
```
Meow. I wake up and claim my territory.
Meow: Territory 'MeowMeow' is ready. IP: 192.168.4.1
Meow. I am ready for paw commands.
```

### 5. Check behavior

The ESP32 creates an open WiFi (SSID: MeowMeow). Connect to it and open `http://192.168.4.1`.
Android should show the captive portal automatically; if not, open the address manually.
There you can toggle the LED with a paw tap.

## Customize firmware

```bash
# Edit src/main.cpp
pio run -e esp32 -t upload   # Build + upload
pio device monitor           # Watch serial output
```

### Configure LED and AP

Edit `src/main.cpp`:

```cpp
const int LED_PIN = LED_BUILTIN;
const uint8_t LED_ON_LEVEL = HIGH;
const char* AP_SSID = "MeowMeow";
```

## Troubleshooting

### Upload fails

- Check USB cable (data cable, not charge-only)
- Hold the BOOT button during upload
- Find port: `pio device list`
- Specify port manually: `pio run -t upload --upload-port /dev/ttyUSB0`

### LED does not toggle

- Check the pin (board-specific)
- Adjust `LED_BUILTIN` in `src/main.cpp`
- Check the board/environment in `platformio.ini`

## Next steps

- [README.md](README.md): Full documentation
- [AGENTS.md](AGENTS.md): Instructions for AI agents
