#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ctype.h>

#include "web_files.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 4
#endif

const int DEFAULT_LED_PIN = LED_BUILTIN;
const uint8_t LED_ON_LEVEL = HIGH;
const uint8_t LED_OFF_LEVEL = LOW;
const uint16_t DEFAULT_MQTT_PORT = 1883;
const char* DEFAULT_MQTT_TOPIC = "meow/lamp";
const char* DEFAULT_MODE = "static";
const uint8_t BOOT_BLINK_COUNT = 2;
const uint16_t BOOT_BLINK_ON_MS = 160;
const uint16_t BOOT_BLINK_OFF_MS = 140;

const char* AP_SSID = "MeowMeow";
const byte DNS_PORT = 53;

WebServer server(80);
DNSServer dnsServer;
Preferences prefs;
bool ledOn = false;
int ledPin = DEFAULT_LED_PIN;
String currentMode = DEFAULT_MODE;

struct DeviceSettings {
    bool wifiEnabled;
    String wifiSsid;
    String wifiPassword;
    bool mqttEnabled;
    String mqttHost;
    uint16_t mqttPort;
    String mqttTopic;
    int ledPin;
};

DeviceSettings settings;

struct LampEffectState {
    unsigned long nextMs;
    int step;
    int flickersRemaining;
    bool outputOn;
};

LampEffectState effect = {0, 0, 0, false};

const uint32_t BLINK_ON_MS = 650;
const uint32_t BLINK_OFF_MS = 650;
const uint32_t PURR_PATTERN_MS[] = {160, 90, 220, 520};
const bool PURR_PATTERN_ON[] = {true, false, true, false};
const size_t PURR_PATTERN_LEN = sizeof(PURR_PATTERN_MS) / sizeof(PURR_PATTERN_MS[0]);
const uint32_t BZZZ_GAP_MIN_MS = 6000;
const uint32_t BZZZ_GAP_MAX_MS = 14000;
const uint16_t BZZZ_FLICKER_MIN_MS = 50;
const uint16_t BZZZ_FLICKER_MAX_MS = 120;
const uint8_t BZZZ_FLICKER_MIN_COUNT = 3;
const uint8_t BZZZ_FLICKER_MAX_COUNT = 4;

void writeLampOutput(bool on, bool force = false) {
    if (!force && effect.outputOn == on) {
        return;
    }
    effect.outputOn = on;
    digitalWrite(ledPin, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

void resetEffectState() {
    effect.nextMs = 0;
    effect.step = 0;
    effect.flickersRemaining = 0;
    effect.outputOn = !ledOn;
    writeLampOutput(ledOn, true);
}

void setLamp(bool on) {
    ledOn = on;
    resetEffectState();
}

void blinkBootSignal() {
    for (uint8_t i = 0; i < BOOT_BLINK_COUNT; i++) {
        digitalWrite(ledPin, LED_ON_LEVEL);
        delay(BOOT_BLINK_ON_MS);
        digitalWrite(ledPin, LED_OFF_LEVEL);
        if (i + 1 < BOOT_BLINK_COUNT) {
            delay(BOOT_BLINK_OFF_MS);
        }
    }
}

int skipJsonWhitespace(const String& input, int index) {
    while (index < static_cast<int>(input.length()) && isspace(static_cast<unsigned char>(input[index]))) {
        index++;
    }
    return index;
}

int findJsonValueStart(const String& input, const char* key) {
    const String pattern = String("\"") + key + "\"";
    int keyPos = input.indexOf(pattern);
    if (keyPos < 0) {
        return -1;
    }
    int colonPos = input.indexOf(':', keyPos + pattern.length());
    if (colonPos < 0) {
        return -1;
    }
    return skipJsonWhitespace(input, colonPos + 1);
}

bool parseJsonStringAt(const String& input, int index, String* out) {
    if (!out) {
        return false;
    }
    if (index >= static_cast<int>(input.length()) || input[index] != '"') {
        return false;
    }
    String result;
    bool escape = false;
    for (int i = index + 1; i < static_cast<int>(input.length()); i++) {
        char c = input[i];
        if (escape) {
            switch (c) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += c;
                    break;
            }
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            *out = result;
            return true;
        }
        result += c;
    }
    return false;
}

bool parseJsonBoolAt(const String& input, int index, bool* out) {
    if (!out) {
        return false;
    }
    if (input.startsWith("true", index)) {
        *out = true;
        return true;
    }
    if (input.startsWith("false", index)) {
        *out = false;
        return true;
    }
    return false;
}

bool parseJsonIntAt(const String& input, int index, int* out) {
    if (!out) {
        return false;
    }
    int i = index;
    bool negative = false;
    if (i < static_cast<int>(input.length()) && input[i] == '-') {
        negative = true;
        i++;
    }
    long value = 0;
    bool hasDigits = false;
    while (i < static_cast<int>(input.length()) && isdigit(static_cast<unsigned char>(input[i]))) {
        hasDigits = true;
        value = value * 10 + (input[i] - '0');
        i++;
    }
    if (!hasDigits) {
        return false;
    }
    if (negative) {
        value = -value;
    }
    *out = static_cast<int>(value);
    return true;
}

bool getJsonBool(const String& input, const char* key, bool* out, bool* found) {
    int start = findJsonValueStart(input, key);
    if (start < 0) {
        if (found) {
            *found = false;
        }
        return true;
    }
    if (found) {
        *found = true;
    }
    return parseJsonBoolAt(input, start, out);
}

bool getJsonInt(const String& input, const char* key, int* out, bool* found) {
    int start = findJsonValueStart(input, key);
    if (start < 0) {
        if (found) {
            *found = false;
        }
        return true;
    }
    if (found) {
        *found = true;
    }
    return parseJsonIntAt(input, start, out);
}

bool getJsonString(const String& input, const char* key, String* out, bool* found) {
    int start = findJsonValueStart(input, key);
    if (start < 0) {
        if (found) {
            *found = false;
        }
        return true;
    }
    if (found) {
        *found = true;
    }
    return parseJsonStringAt(input, start, out);
}

String jsonEscape(const String& input) {
    String output;
    output.reserve(input.length() + 8);
    for (int i = 0; i < static_cast<int>(input.length()); i++) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        switch (c) {
            case '\"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    output += buf;
                } else {
                    output += static_cast<char>(c);
                }
                break;
        }
    }
    return output;
}

bool isValidMode(const String& mode) {
    return mode == "static" || mode == "purr" || mode == "bzzz" || mode == "blink";
}

bool isTimeReached(unsigned long now, unsigned long target) {
    return static_cast<long>(now - target) >= 0;
}

void updateLampEffect() {
    if (!ledOn) {
        writeLampOutput(false);
        return;
    }

    if (currentMode == "static") {
        writeLampOutput(true);
        return;
    }

    const unsigned long now = millis();

    if (currentMode == "blink") {
        if (effect.nextMs == 0) {
            writeLampOutput(true);
            effect.nextMs = now + BLINK_ON_MS;
            return;
        }

        if (isTimeReached(now, effect.nextMs)) {
            if (effect.outputOn) {
                writeLampOutput(false);
                effect.nextMs = now + BLINK_OFF_MS;
            } else {
                writeLampOutput(true);
                effect.nextMs = now + BLINK_ON_MS;
            }
        }
        return;
    }

    if (currentMode == "purr") {
        if (effect.nextMs == 0) {
            effect.step = 0;
            writeLampOutput(PURR_PATTERN_ON[0]);
            effect.nextMs = now + PURR_PATTERN_MS[0];
            return;
        }

        if (isTimeReached(now, effect.nextMs)) {
            effect.step = (effect.step + 1) % static_cast<int>(PURR_PATTERN_LEN);
            writeLampOutput(PURR_PATTERN_ON[effect.step]);
            effect.nextMs = now + PURR_PATTERN_MS[effect.step];
        }
        return;
    }

    if (currentMode == "bzzz") {
        if (effect.nextMs == 0) {
            writeLampOutput(true);
            effect.flickersRemaining = 0;
            effect.nextMs = now + random(BZZZ_GAP_MIN_MS, BZZZ_GAP_MAX_MS + 1);
            return;
        }

        if (effect.flickersRemaining == 0) {
            if (isTimeReached(now, effect.nextMs)) {
                const uint8_t flickers = random(BZZZ_FLICKER_MIN_COUNT, BZZZ_FLICKER_MAX_COUNT + 1);
                effect.flickersRemaining = flickers * 2;
                effect.nextMs = now;
            } else {
                writeLampOutput(true);
                return;
            }
        }

        if (isTimeReached(now, effect.nextMs)) {
            writeLampOutput(!effect.outputOn);
            effect.flickersRemaining--;
            effect.nextMs = now + random(BZZZ_FLICKER_MIN_MS, BZZZ_FLICKER_MAX_MS + 1);

            if (effect.flickersRemaining <= 0) {
                writeLampOutput(true);
                effect.nextMs = now + random(BZZZ_GAP_MIN_MS, BZZZ_GAP_MAX_MS + 1);
            }
        }
        return;
    }

    writeLampOutput(true);
}

void loadSettingsFromPrefs() {
    settings.wifiEnabled = prefs.getBool("wifi_en", false);
    settings.wifiSsid = prefs.getString("wifi_ssid", "");
    settings.wifiPassword = prefs.getString("wifi_pass", "");
    settings.mqttEnabled = prefs.getBool("mqtt_en", false);
    settings.mqttHost = prefs.getString("mqtt_host", "");
    settings.mqttPort = static_cast<uint16_t>(prefs.getUInt("mqtt_port", DEFAULT_MQTT_PORT));
    settings.mqttTopic = prefs.getString("mqtt_topic", DEFAULT_MQTT_TOPIC);
    settings.ledPin = prefs.getInt("led_pin", DEFAULT_LED_PIN);

    if (settings.ledPin < 0 || settings.ledPin > 40) {
        settings.ledPin = DEFAULT_LED_PIN;
    }
    if (settings.mqttPort == 0 || settings.mqttPort > 65535) {
        settings.mqttPort = DEFAULT_MQTT_PORT;
    }

    currentMode = prefs.getString("mode", DEFAULT_MODE);
    if (!isValidMode(currentMode)) {
        currentMode = DEFAULT_MODE;
    }
}

void saveSettingsToPrefs() {
    prefs.putBool("wifi_en", settings.wifiEnabled);
    prefs.putString("wifi_ssid", settings.wifiSsid);
    prefs.putString("wifi_pass", settings.wifiPassword);
    prefs.putBool("mqtt_en", settings.mqttEnabled);
    prefs.putString("mqtt_host", settings.mqttHost);
    prefs.putUInt("mqtt_port", settings.mqttPort);
    prefs.putString("mqtt_topic", settings.mqttTopic);
    prefs.putInt("led_pin", settings.ledPin);
    prefs.putString("mode", currentMode);
}

void applyLedPin(int newPin) {
    if (newPin < 0 || newPin > 40) {
        return;
    }
    if (newPin == ledPin) {
        return;
    }
    int previousPin = ledPin;
    ledPin = newPin;
    pinMode(ledPin, OUTPUT);
    writeLampOutput(effect.outputOn, true);
    if (previousPin != ledPin) {
        pinMode(previousPin, INPUT);
    }
}

String settingsToJson() {
    String payload = "{";
    payload += "\"wifi_enabled\":";
    payload += settings.wifiEnabled ? "true" : "false";
    payload += ",\"wifi_ssid\":\"";
    payload += jsonEscape(settings.wifiSsid);
    payload += "\",\"wifi_password\":\"";
    payload += jsonEscape(settings.wifiPassword);
    payload += "\",\"mqtt_enabled\":";
    payload += settings.mqttEnabled ? "true" : "false";
    payload += ",\"mqtt_host\":\"";
    payload += jsonEscape(settings.mqttHost);
    payload += "\",\"mqtt_port\":";
    payload += settings.mqttPort;
    payload += ",\"mqtt_topic\":\"";
    payload += jsonEscape(settings.mqttTopic);
    payload += "\",\"led_pin\":";
    payload += settings.ledPin;
    payload += "}";
    return payload;
}

const WebFile* findWebFile(const String& path) {
    for (size_t i = 0; i < webFilesCount; i++) {
        if (path == webFiles[i].path) {
            return &webFiles[i];
        }
    }
    return nullptr;
}

bool serveWebFile(const String& path) {
    String target = path;
    if (target == "/") {
        target = "/index.html";
    }

    const WebFile* file = findWebFile(target);
    if (!file) {
        return false;
    }

    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, file->mime_type, reinterpret_cast<const char*>(file->data), file->size);
    return true;
}

String portalUrl() {
    return String("http://") + WiFi.softAPIP().toString() + "/";
}

void redirectToPortal() {
    server.sendHeader("Location", portalUrl());
    server.send(302, "text/plain", "Meow.");
}

void sendStatus() {
    char payload[200];
    const unsigned long uptimeSeconds = millis() / 1000;
    snprintf(payload, sizeof(payload),
             "{\"led_on\":%s,\"uptime_s\":%lu,\"ssid\":\"%s\",\"mode\":\"%s\"}",
             ledOn ? "true" : "false",
             uptimeSeconds,
             AP_SSID,
             currentMode.c_str());
    server.send(200, "application/json", payload);
}

bool parseDesiredState(const String& input, bool* out) {
    if (!out) {
        return false;
    }
    String value = input;
    value.trim();
    value.toLowerCase();
    if (value == "on" || value == "1" || value == "true") {
        *out = true;
        return true;
    }
    if (value == "off" || value == "0" || value == "false") {
        *out = false;
        return true;
    }
    if (value == "toggle") {
        *out = !ledOn;
        return true;
    }
    return false;
}

void handleSetLamp() {
    String rawState = server.arg("state");
    if (rawState.isEmpty()) {
        rawState = server.arg("plain");
    }

    bool desiredState = ledOn;
    if (rawState.isEmpty()) {
        desiredState = !ledOn;
    } else if (!parseDesiredState(rawState, &desiredState)) {
        server.send(400, "application/json", "{\"error\":\"unknown_state\"}");
        return;
    }

    setLamp(desiredState);
    sendStatus();
}

void handleGetSettings() {
    server.send(200, "application/json", settingsToJson());
}

void handleSaveSettings() {
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    bool found = false;
    bool valueBool = false;
    int valueInt = 0;
    String valueStr;

    if (!getJsonBool(body, "wifi_enabled", &valueBool, &found)) {
        server.send(400, "application/json", "{\"error\":\"wifi_enabled\"}");
        return;
    }
    if (found) {
        settings.wifiEnabled = valueBool;
    }

    if (!getJsonString(body, "wifi_ssid", &valueStr, &found)) {
        server.send(400, "application/json", "{\"error\":\"wifi_ssid\"}");
        return;
    }
    if (found) {
        settings.wifiSsid = valueStr;
    }

    if (!getJsonString(body, "wifi_password", &valueStr, &found)) {
        server.send(400, "application/json", "{\"error\":\"wifi_password\"}");
        return;
    }
    if (found) {
        settings.wifiPassword = valueStr;
    }

    if (!getJsonBool(body, "mqtt_enabled", &valueBool, &found)) {
        server.send(400, "application/json", "{\"error\":\"mqtt_enabled\"}");
        return;
    }
    if (found) {
        settings.mqttEnabled = valueBool;
    }

    if (!getJsonString(body, "mqtt_host", &valueStr, &found)) {
        server.send(400, "application/json", "{\"error\":\"mqtt_host\"}");
        return;
    }
    if (found) {
        settings.mqttHost = valueStr;
    }

    if (!getJsonInt(body, "mqtt_port", &valueInt, &found)) {
        server.send(400, "application/json", "{\"error\":\"mqtt_port\"}");
        return;
    }
    if (found) {
        if (valueInt > 0 && valueInt <= 65535) {
            settings.mqttPort = static_cast<uint16_t>(valueInt);
        }
    }

    if (!getJsonString(body, "mqtt_topic", &valueStr, &found)) {
        server.send(400, "application/json", "{\"error\":\"mqtt_topic\"}");
        return;
    }
    if (found) {
        settings.mqttTopic = valueStr;
    }

    if (!getJsonInt(body, "led_pin", &valueInt, &found)) {
        server.send(400, "application/json", "{\"error\":\"led_pin\"}");
        return;
    }
    if (found) {
        if (valueInt >= 0 && valueInt <= 40) {
            settings.ledPin = valueInt;
            applyLedPin(settings.ledPin);
        }
    }

    saveSettingsToPrefs();
    handleGetSettings();
}

void handleSetMode() {
    String body = server.arg("plain");
    if (body.isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    bool found = false;
    String valueStr;
    if (!getJsonString(body, "mode", &valueStr, &found) || !found) {
        server.send(400, "application/json", "{\"error\":\"mode\"}");
        return;
    }

    valueStr.toLowerCase();
    if (!isValidMode(valueStr)) {
        server.send(400, "application/json", "{\"error\":\"mode\"}");
        return;
    }

    currentMode = valueStr;
    prefs.putString("mode", currentMode);
    resetEffectState();
    server.send(200, "application/json", String("{\"mode\":\"") + currentMode + "\"}");
}

void setupRoutes() {
    server.on("/api/paw", HTTP_GET, []() { sendStatus(); });
    server.on("/api/paw", HTTP_POST, []() { handleSetLamp(); });
    server.on("/api/settings", HTTP_GET, []() { handleGetSettings(); });
    server.on("/api/settings", HTTP_POST, []() { handleSaveSettings(); });
    server.on("/api/mode", HTTP_POST, []() { handleSetMode(); });

    server.on("/generate_204", HTTP_GET, []() { redirectToPortal(); });
    server.on("/gen_204", HTTP_GET, []() { redirectToPortal(); });
    server.on("/hotspot-detect.html", HTTP_GET, []() { redirectToPortal(); });
    server.on("/ncsi.txt", HTTP_GET, []() { redirectToPortal(); });
    server.on("/success.txt", HTTP_GET, []() { redirectToPortal(); });
    server.on("/fwlink", HTTP_GET, []() { redirectToPortal(); });

    server.onNotFound([]() {
        if (server.uri().startsWith("/api/")) {
            server.send(404, "application/json", "{\"error\":\"unknown_api\"}");
            return;
        }
        if (serveWebFile(server.uri())) {
            return;
        }
        redirectToPortal();
    });
}

void setupAccessPoint() {
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(AP_SSID)) {
        IPAddress ip = WiFi.softAPIP();
        Serial.printf("Meow: Territory '%s' is ready. IP: %s\n", AP_SSID, ip.toString().c_str());
    } else {
        Serial.println("Meow: Could not open my territory.");
    }
}

void setupCaptivePortal() {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println("Meow: I route every track to my bowl.");
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Meow. I wake up and claim my territory.");
    randomSeed(static_cast<unsigned long>(micros()));

    prefs.begin("meowlamp", false);
    loadSettingsFromPrefs();
    ledPin = settings.ledPin;
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LED_OFF_LEVEL);
    blinkBootSignal();
    setLamp(false);

    setupAccessPoint();
    setupCaptivePortal();
    setupRoutes();
    server.begin();
    Serial.println("Meow. I am ready for paw commands.");
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    updateLampEffect();
}
