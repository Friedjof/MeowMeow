# MeowMeow ESP32 PlatformIO Template 😺🐾

Ich bin dein schnurrender Firmware-Kater: ein schlankes PlatformIO-Template mit
Cat-Portal, Web-UI und LED-Lampe. Ich schnurre in `src/main.cpp` und bringe mein
Revier als SoftAP ins Wohnzimmer. 🐈‍⬛✨

<p align="center">
  <img src="docs/assets/meowmeow.png" alt="MeowMeow web UI" width="420">
</p>

## Info ℹ️

> 🐾 **Wer ich bin:** Ein ESP32-Template, das als offenes WLAN startet und eine
>    kleine Web-UI fuer die Lampen-LED bereitstellt.
> 🧶 **Warum ich da bin:** Schnell loslegen, ohne Ballast, mit einem UI, das in
>    die Firmware eingebettet wird.
> 🥣 **Was ich brauche:** PlatformIO Core, ein ESP32-Board, USB-Kabel; Node.js
>    nur, wenn du am Web-UI schraubst.

## Was ich kann (aus Katzensicht) 😼

- 🌐 SoftAP + DNS-Captive-Portal, damit du automatisch bei mir landest.
- 💡 Lampen-LED an/aus plus Effekte: static, blink, purr, bzzz.
- 🧩 JSON-API fuer Status, Einstellungen und Modus.
- 🧪 Multi-Target Builds: esp32, esp32c3, esp32s3, esp32c6.
- 🧶 Web-UI mit Vite, als C-Header in die Firmware eingebettet.
- 🧺 Optionales Filesystem via `data-template/` und Makefile-Targets.

## Projektlayout 🗂️

```
.
├── boards/                 # Partition schemes
├── data-template/          # Filesystem template (optional)
├── docs/
│   └── assets/             # README images and diagrams
├── include/                # Header files
├── lib/                    # Custom libs + generated web headers
├── src/                    # ESP32 firmware source
├── test/                   # Unit tests
├── tools/                  # Build and setup tools
├── web/                    # Vite web UI source
├── Makefile                # Build helpers
└── platformio.ini          # PlatformIO configuration
```

## Schnellstart 🐾

1) Setup:

```bash
./tools/setup.sh
```

2) Build:

```bash
pio run -e esp32
```

3) Flash:

```bash
pio run -e esp32 -t upload
```

4) Monitor (optional):

```bash
pio device monitor
```

Oder mit Makefile:

```bash
make build BOARD=esp32
make flash
make monitor
```

## Mit dem Revier verbinden 🌐

Ich starte ein offenes WLAN (SSID: `MeowMeow`). Verbinde dich und oeffne
`http://192.168.4.1`. Android zeigt meist automatisch das Portal; sonst einfach
manuell oeffnen. Dann kannst du die Lampe mit einem Pfotentipp steuern. 🐾

## API mit Pfotenabdruck 🐾

- `GET /api/paw` liefert den Status:
  `{"led_on":true,"uptime_s":123,"ssid":"MeowMeow","mode":"static"}`
- `POST /api/paw` setzt den Lampenstatus per `state` oder Body.
  Akzeptiert: `on`, `off`, `toggle`, `true`, `false`, `1`, `0`.
- `GET /api/settings` liefert gespeicherte Einstellungen als JSON.
- `POST /api/settings` akzeptiert JSON mit:
  `wifi_enabled`, `wifi_ssid`, `wifi_password`, `mqtt_enabled`, `mqtt_host`,
  `mqtt_port`, `mqtt_topic`, `led_pin`.
- `POST /api/mode` akzeptiert `{"mode":"static"}` mit:
  `static`, `blink`, `purr`, `bzzz`.

Einstellungen landen im NVS (Preferences). WLAN- und MQTT-Felder werden aktuell
nur gespeichert, nicht automatisch verbunden. 🐱‍👓

## Firmware anpassen 🛠️

Die wichtigsten Defaults in `src/main.cpp`:

```cpp
const int DEFAULT_LED_PIN = LED_BUILTIN;
const uint8_t LED_ON_LEVEL = HIGH;
const char* AP_SSID = "MeowMeow";
const char* DEFAULT_MODE = "static";
```

## Web-UI Entwicklung 🧵

Die UI in `web/` wird zu C-Headern gebaut und in die Firmware eingebettet.

```bash
make web-headers
```

Oder direkt im Web-Ordner:

```bash
make -C web dev
make -C web build-esp
```

## Filesystem (optional) 📁

- `data-template/` ist das Template.
- `./tools/setup.sh` kopiert es beim ersten Setup nach `data/`.
- Upload mit `make deploy-fs` oder `pio run -t uploadfs`.

## Partitionen 🧱

`platformio.ini` nutzt `boards/min_spiffs.csv` fuer OTA und ein kleines FS.
Passe die CSV in `boards/` an, wenn du mehr Platz brauchst. 🐾

## Kleine Katzen-Details ✨

- Ich miaue beim Booten im Serial-Log und verrate meine IP. 😺
- Das Captive Portal faengt typische OS-Checks ab (`/generate_204`, `fwlink`).
- `make deploy-flash` erledigt Web-UI, Firmware und Filesystem in einem Rutsch.
- Ich mag kurze, nicht-blockierende Schleifen (lies: `loop()` bleibt flink).

## Doku & Hinweise 📌

- README-Bilder liegen in `docs/assets/` und werden relativ verlinkt.
- Fuer AI-Agenten: [AGENTS.md](AGENTS.md).
- Kurzer Einstieg: [QUICKSTART.md](QUICKSTART.md).
