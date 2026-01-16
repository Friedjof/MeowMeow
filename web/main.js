const API_ENDPOINT = '/api/paw';
const SETTINGS_ENDPOINT = '/api/settings';
const MODE_ENDPOINT = '/api/mode';
const params = new URLSearchParams(window.location.search);
const MOCK_MODE =
    params.has('mock') ||
    window.location.hostname === 'localhost' ||
    window.location.hostname === '127.0.0.1' ||
    window.location.protocol === 'file:';

const defaultSettings = {
    wifi_enabled: false,
    wifi_ssid: '',
    wifi_password: '',
    mqtt_enabled: false,
    mqtt_host: '',
    mqtt_port: 1883,
    mqtt_topic: 'meow/lamp',
    led_pin: 4
};

let ledOn = false;
let currentMode = 'static';
let settingsState = { ...defaultSettings };

const mockApi = (() => {
    const startTime = Date.now();
    const store = {
        led_on: false,
        ssid: 'MeowMeow',
        mode: 'static',
        settings: { ...defaultSettings }
    };

    function status() {
        return {
            led_on: store.led_on,
            uptime_s: Math.floor((Date.now() - startTime) / 1000),
            ssid: store.ssid,
            mode: store.mode
        };
    }

    return {
        getStatus() {
            return Promise.resolve(status());
        },
        setLed(on) {
            store.led_on = on;
            return Promise.resolve(status());
        },
        setMode(mode) {
            store.mode = mode;
            return Promise.resolve({ mode: store.mode });
        },
        getSettings() {
            return Promise.resolve({ ...store.settings });
        },
        saveSettings(payload) {
            store.settings = { ...store.settings, ...payload };
            return Promise.resolve({ ...store.settings });
        }
    };
})();

const liveApi = {
    async getStatus() {
        const response = await fetch(API_ENDPOINT, { cache: 'no-store' });
        if (!response.ok) {
            throw new Error('status');
        }
        return response.json();
    },
    async setLed(on) {
        const body = new URLSearchParams({ state: on ? 'on' : 'off' });
        const response = await fetch(API_ENDPOINT, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body
        });
        if (!response.ok) {
            throw new Error('toggle');
        }
        return response.json();
    },
    async getSettings() {
        const response = await fetch(SETTINGS_ENDPOINT, { cache: 'no-store' });
        if (!response.ok) {
            throw new Error('settings');
        }
        return response.json();
    },
    async saveSettings(payload) {
        const response = await fetch(SETTINGS_ENDPOINT, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });
        if (!response.ok) {
            throw new Error('settings');
        }
        return response.json();
    },
    async setMode(mode) {
        const response = await fetch(MODE_ENDPOINT, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ mode })
        });
        if (!response.ok) {
            throw new Error('mode');
        }
        return response.json();
    }
};

const api = MOCK_MODE ? mockApi : liveApi;

document.addEventListener('DOMContentLoaded', () => {
    bindUI();
    setupCatEyes();
    applyVersion();
    setModePill();
    updateModeUI();
    refreshStatus();
    loadSettings();
    setInterval(refreshStatus, 5000);
});

function bindUI() {
    const toggleBtn = document.getElementById('toggle-btn');
    const openSettings = document.getElementById('open-settings');
    const closeSettings = document.getElementById('close-settings');
    const backdrop = document.getElementById('backdrop');
    const modeSelect = document.getElementById('mode-select');

    toggleBtn.addEventListener('click', () => setLedState(!ledOn));
    openSettings.addEventListener('click', () => toggleSettings(true));
    closeSettings.addEventListener('click', () => toggleSettings(false));
    backdrop.addEventListener('click', () => toggleSettings(false));
    modeSelect.addEventListener('change', (event) => handleModeChange(event.target.value));

    document.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') {
            toggleSettings(false);
        }
    });

    const wifiEnabled = document.getElementById('wifi-enabled');
    const wifiSsid = document.getElementById('wifi-ssid');
    const mqttEnabled = document.getElementById('mqtt-enabled');
    const mqttHost = document.getElementById('mqtt-host');
    const mqttPort = document.getElementById('mqtt-port');
    const mqttTopic = document.getElementById('mqtt-topic');
    const ledPin = document.getElementById('led-pin');

    wifiEnabled.addEventListener('change', syncSettingsUI);
    wifiSsid.addEventListener('input', syncSettingsUI);
    mqttEnabled.addEventListener('change', syncSettingsUI);

    mqttHost.addEventListener('input', () => updateSettingsState());
    mqttPort.addEventListener('input', () => updateSettingsState());
    mqttTopic.addEventListener('input', () => updateSettingsState());
    ledPin.addEventListener('input', () => updateSettingsState());

    const settingsForm = document.getElementById('settings-form');
    settingsForm.addEventListener('submit', saveSettings);

    const resetButton = document.getElementById('reset-settings');
    resetButton.addEventListener('click', () => {
        applySettings({ ...defaultSettings });
        setSaveStatus('Reset to defaults.');
    });

    const ipPill = document.getElementById('ip-pill');
    const host = window.location.hostname;
    if (host) {
        ipPill.textContent = `IP: ${host}`;
    }
}

function setModePill() {
    const modePill = document.getElementById('mode-pill');
    if (!modePill) {
        return;
    }
    if (MOCK_MODE) {
        modePill.textContent = 'Mode: mock';
    } else {
        modePill.textContent = 'Mode: live';
    }
}

function applyVersion() {
    const version = typeof __APP_VERSION__ !== 'undefined' ? __APP_VERSION__ : 'dev';
    const versionElement = document.getElementById('uiVersionFooter');
    if (!versionElement) {
        return;
    }
    versionElement.textContent = version;
    versionElement.href = `https://github.com/Friedjof/MeowMeow/releases/tag/${version}`;
}

async function refreshStatus() {
    try {
        const data = await api.getStatus();
        updateConnection(MOCK_MODE ? 'mock' : 'ok');
        applyStatus(data);
    } catch (error) {
        updateConnection('down');
    }
}

async function setLedState(desired) {
    try {
        const data = await api.setLed(desired);
        updateConnection(MOCK_MODE ? 'mock' : 'ok');
        applyStatus(data);
    } catch (error) {
        updateConnection('down');
    }
}

function applyStatus(data) {
    if (typeof data.led_on === 'boolean') {
        ledOn = data.led_on;
    }

    if (typeof data.mode === 'string') {
        currentMode = data.mode;
    }

    if (typeof data.uptime_s === 'number') {
        document.getElementById('uptime').textContent = formatUptime(data.uptime_s);
    }

    if (typeof data.ssid === 'string' && data.ssid.length > 0) {
        document.getElementById('ssid-pill').textContent = `SSID: ${data.ssid}`;
    }

    const panel = document.getElementById('panel');
    panel.dataset.led = ledOn ? 'on' : 'off';
    document.body.classList.toggle('lamp-on', ledOn);

    document.getElementById('led-state').textContent = ledOn ? 'on' : 'off';

    const toggleBtn = document.getElementById('toggle-btn');
    toggleBtn.dataset.state = ledOn ? 'on' : 'off';
    toggleBtn.setAttribute('aria-pressed', ledOn ? 'true' : 'false');

    document.getElementById('toggle-text').textContent = ledOn
        ? 'Paw to switch off'
        : 'Paw to switch on';

    updateModeUI();
}

function updateConnection(state) {
    const panel = document.getElementById('panel');
    panel.dataset.connection = state;

    const status = document.getElementById('connection-status');
    if (state === 'ok') {
        status.textContent = 'awake';
    } else if (state === 'mock') {
        status.textContent = 'dreaming';
    } else {
        status.textContent = 'sniffing...';
    }
}

function setupCatEyes() {
    const cat = document.getElementById('cat');
    if (!cat) {
        return;
    }
    const eyes = Array.from(cat.querySelectorAll('.eye'));
    if (!eyes.length) {
        return;
    }

    const prefersReduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

    const setPupilOffset = (dx, dy) => {
        eyes.forEach((eye) => {
            eye.style.setProperty('--px', `${dx}px`);
            eye.style.setProperty('--py', `${dy}px`);
        });
    };

    const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

    cat.addEventListener('pointermove', (event) => {
        const rect = cat.getBoundingClientRect();
        const x = (event.clientX - rect.left) / rect.width - 0.5;
        const y = (event.clientY - rect.top) / rect.height - 0.5;
        setPupilOffset(clamp(x * 6, -6, 6), clamp(y * 6, -6, 6));
    });

    cat.addEventListener('pointerleave', () => {
        setPupilOffset(0, 0);
    });

    if (prefersReduced) {
        return;
    }

    const blinkOnce = () => {
        eyes.forEach((eye) => eye.classList.add('blink'));
        window.setTimeout(() => {
            eyes.forEach((eye) => eye.classList.remove('blink'));
        }, 140);
    };

    const scheduleBlink = () => {
        const delay = 2800 + Math.random() * 2200;
        window.setTimeout(() => {
            blinkOnce();
            scheduleBlink();
        }, delay);
    };

    scheduleBlink();
}

function updateModeUI() {
    const modeSelect = document.getElementById('mode-select');
    const modeHint = document.getElementById('mode-hint');
    const modePanel = document.getElementById('mode-panel');
    if (!modeSelect || !modeHint) {
        return;
    }

    modeSelect.disabled = !ledOn;
    if (modePanel) {
        modePanel.classList.toggle('is-disabled', !ledOn);
    }
    if (!modeSelect.querySelector(`option[value="${currentMode}"]`)) {
        currentMode = 'static';
    }
    modeSelect.value = currentMode;

    if (!ledOn) {
        modeHint.textContent = 'Turn the lamp on to choose a mood.';
    } else {
        modeHint.textContent = 'Choose how I glow.';
    }
}

async function handleModeChange(mode) {
    currentMode = mode;
    updateModeUI();

    if (!ledOn) {
        return;
    }

    try {
        await api.setMode(mode);
    } catch (error) {
        const modeHint = document.getElementById('mode-hint');
        if (modeHint) {
            modeHint.textContent = MOCK_MODE
                ? 'Mock mode: no device to sync.'
                : 'Mode will sync once the lamp is ready.';
        }
    }
}

async function loadSettings() {
    try {
        const data = await api.getSettings();
        applySettings(data);
    } catch (error) {
        applySettings({ ...defaultSettings });
        setSaveStatus(MOCK_MODE ? 'Mock settings ready.' : 'Could not load settings.');
    }
}

function applySettings(data) {
    settingsState = { ...defaultSettings, ...data };

    document.getElementById('wifi-enabled').checked = !!settingsState.wifi_enabled;
    document.getElementById('wifi-ssid').value = settingsState.wifi_ssid || '';
    document.getElementById('wifi-pass').value = settingsState.wifi_password || '';

    document.getElementById('mqtt-enabled').checked = !!settingsState.mqtt_enabled;
    document.getElementById('mqtt-host').value = settingsState.mqtt_host || '';
    document.getElementById('mqtt-port').value = settingsState.mqtt_port || 1883;
    document.getElementById('mqtt-topic').value = settingsState.mqtt_topic || '';

    document.getElementById('led-pin').value = settingsState.led_pin ?? '';

    syncSettingsUI();
}

function updateSettingsState() {
    settingsState = collectSettings();
}

function collectSettings() {
    return {
        wifi_enabled: document.getElementById('wifi-enabled').checked,
        wifi_ssid: document.getElementById('wifi-ssid').value.trim(),
        wifi_password: document.getElementById('wifi-pass').value,
        mqtt_enabled: document.getElementById('mqtt-enabled').checked,
        mqtt_host: document.getElementById('mqtt-host').value.trim(),
        mqtt_port: Number(document.getElementById('mqtt-port').value || 1883),
        mqtt_topic: document.getElementById('mqtt-topic').value.trim(),
        led_pin: Number(document.getElementById('led-pin').value || defaultSettings.led_pin)
    };
}

function syncSettingsUI() {
    const wifiEnabled = document.getElementById('wifi-enabled').checked;
    const wifiSsid = document.getElementById('wifi-ssid').value.trim();
    const wifiFields = document.getElementById('wifi-fields');

    wifiFields.disabled = !wifiEnabled;
    wifiFields.classList.toggle('is-disabled', !wifiEnabled);

    const mqttSection = document.getElementById('mqtt-section');
    const mqttToggle = document.getElementById('mqtt-enabled');
    const mqttFields = document.getElementById('mqtt-fields');
    const mqttNote = document.getElementById('mqtt-note');

    const wifiReady = wifiEnabled && wifiSsid.length > 0;

    if (!wifiReady) {
        mqttToggle.checked = false;
    }

    mqttToggle.disabled = !wifiReady;
    mqttFields.disabled = !wifiReady || !mqttToggle.checked;
    mqttFields.classList.toggle('is-disabled', mqttFields.disabled);

    if (wifiReady) {
        mqttNote.textContent = 'Tell me which topic to listen to.';
    } else {
        mqttNote.textContent = 'Add external WiFi to unlock MQTT.';
    }

    mqttSection.classList.toggle('is-locked', !wifiReady);

    updateSettingsState();
}

async function saveSettings(event) {
    event.preventDefault();
    const payload = collectSettings();
    setSaveStatus('Saving...');

    try {
        const data = await api.saveSettings(payload);
        applySettings(data);
        setSaveStatus(MOCK_MODE ? 'Saved in mock mode.' : 'Saved.');
        updateConnection(MOCK_MODE ? 'mock' : 'ok');
    } catch (error) {
        setSaveStatus('Could not save settings.');
        updateConnection('down');
    }
}

function toggleSettings(open) {
    const body = document.body;
    const drawer = document.getElementById('settings-drawer');
    const backdrop = document.getElementById('backdrop');
    const openButton = document.getElementById('open-settings');

    if (open) {
        body.classList.add('settings-open');
        drawer.setAttribute('aria-hidden', 'false');
        backdrop.setAttribute('aria-hidden', 'false');
        openButton.setAttribute('aria-expanded', 'true');
    } else {
        body.classList.remove('settings-open');
        drawer.setAttribute('aria-hidden', 'true');
        backdrop.setAttribute('aria-hidden', 'true');
        openButton.setAttribute('aria-expanded', 'false');
    }
}

function setSaveStatus(message) {
    const status = document.getElementById('save-status');
    status.textContent = message;
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);

    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    }
    if (hours > 0) {
        return `${hours}h ${minutes}m`;
    }
    return `${minutes}m`;
}
