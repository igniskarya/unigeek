// device.cpp
#include "core/Device.h"
#include "core/Navigation.h"
#include <Preferences.h>
#include <LittleFS.h>
#include <SD.h>

// Global navigation instance
Navigation nav;

// Forward declarations
void device_load_config();
void device_save_config();
void device_play_tone(int frequency, int duration);

// Device configuration
struct DeviceConfig {
    char deviceName[32] = "DIY Smoochie";
    uint8_t brightness = 128;
    uint8_t volume = 50;
    uint16_t displayTimeout = 30; // seconds
    uint16_t powerTimeout = 60;   // seconds
    bool autoDisplayOff = true;
    bool autoPowerOff = false;
    bool navSound = true;
    uint32_t themeColor = 0x00FF00; // Green default
    NavigationMode navMode = MODE_DEFAULT;
    char webPassword[32] = "admin";
};

static DeviceConfig config;
static Preferences preferences;

// Device initialization
void device_init() {
    Serial.begin(115200);
    delay(100);
    Serial.println("DIY Smoochie Starting...");
    
    // Initialize GPIO for buttons
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    
    // Initialize storage
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
    } else {
        Serial.println("LittleFS mounted");
    }
    
    // Try SD card
    if (SD.begin(4)) { // SD_CS pin typically 4
        Serial.println("SD card mounted");
    } else {
        Serial.println("No SD card detected");
    }
    
    // Load configuration
    device_load_config();
    
    // Initialize navigation
    nav.begin();
    nav.setMode(config.navMode);
    
    // Initialize display (to be implemented per display type)
    // display_init();
    
    Serial.printf("Device: %s ready\n", config.deviceName);
}

void device_load_config() {
    preferences.begin("unigeek", false);
    
    strcpy(config.deviceName, preferences.getString("devName", "DIY Smoochie").c_str());
    config.brightness = preferences.getUChar("bright", 128);
    config.volume = preferences.getUChar("volume", 50);
    config.displayTimeout = preferences.getUShort("dispTO", 30);
    config.powerTimeout = preferences.getUShort("powTO", 60);
    config.autoDisplayOff = preferences.getBool("autoDisp", true);
    config.autoPowerOff = preferences.getBool("autoPow", false);
    config.navSound = preferences.getBool("navSound", true);
    config.themeColor = preferences.getUInt("theme", 0x00FF00);
    config.navMode = (NavigationMode)preferences.getUChar("navMode", MODE_DEFAULT);
    strcpy(config.webPassword, preferences.getString("webPass", "admin").c_str());
    
    preferences.end();
}

void device_save_config() {
    preferences.begin("unigeek", false);
    
    preferences.putString("devName", config.deviceName);
    preferences.putUChar("bright", config.brightness);
    preferences.putUChar("volume", config.volume);
    preferences.putUShort("dispTO", config.displayTimeout);
    preferences.putUShort("powTO", config.powerTimeout);
    preferences.putBool("autoDisp", config.autoDisplayOff);
    preferences.putBool("autoPow", config.autoPowerOff);
    preferences.putBool("navSound", config.navSound);
    preferences.putUInt("theme", config.themeColor);
    preferences.putUChar("navMode", config.navMode);
    preferences.putString("webPass", config.webPassword);
    
    preferences.end();
}

const char* device_get_name() {
    return config.deviceName;
}

void device_set_name(const char* name) {
    strncpy(config.deviceName, name, 31);
    config.deviceName[31] = '\0';
    device_save_config();
}

uint8_t device_get_brightness() {
    return config.brightness;
}

void device_set_brightness(uint8_t value) {
    config.brightness = value;
    device_save_config();
}

uint8_t device_get_volume() {
    return config.volume;
}

void device_set_volume(uint8_t value) {
    config.volume = value;
    device_save_config();
}

uint16_t device_get_display_timeout() {
    return config.displayTimeout;
}

void device_set_display_timeout(uint16_t seconds) {
    config.displayTimeout = seconds;
    device_save_config();
}

uint16_t device_get_power_timeout() {
    return config.powerTimeout;
}

void device_set_power_timeout(uint16_t seconds) {
    config.powerTimeout = seconds;
    device_save_config();
}

bool device_get_auto_display_off() {
    return config.autoDisplayOff;
}

void device_set_auto_display_off(bool enable) {
    config.autoDisplayOff = enable;
    device_save_config();
}

bool device_get_auto_power_off() {
    return config.autoPowerOff;
}

void device_set_auto_power_off(bool enable) {
    config.autoPowerOff = enable;
    device_save_config();
}

bool device_get_nav_sound() {
    return config.navSound;
}

void device_set_nav_sound(bool enable) {
    config.navSound = enable;
    device_save_config();
}

uint32_t device_get_theme_color() {
    return config.themeColor;
}

void device_set_theme_color(uint32_t color) {
    config.themeColor = color;
    device_save_config();
}

NavigationMode device_get_nav_mode() {
    return config.navMode;
}

void device_set_nav_mode(NavigationMode mode) {
    config.navMode = mode;
    nav.setMode(mode);
    device_save_config();
}

const char* device_get_web_password() {
    return config.webPassword;
}

void device_set_web_password(const char* password) {
    strncpy(config.webPassword, password, 31);
    config.webPassword[31] = '\0';
    device_save_config();
}

void device_power_off() {
    Serial.println("Powering off...");
    // Implement power off for your hardware
    // For ESP32: esp_deep_sleep_start() or GPIO control
    esp_deep_sleep_start();
}

void device_play_tone(int frequency, int duration) {
    // Implement if speaker is available
    // For buzzer: ledcWriteTone(0, frequency);
    // delay(duration);
    // ledcWriteTone(0, 0);
}

void device_play_nav_sound() {
    if (config.navSound) {
        device_play_tone(1000, 50);
    }
}

// Navigation implementation
Navigation::Navigation() : _mode(MODE_DEFAULT), _lastAction(NAV_NONE),
    _lastDebounceTime(0), _selectPressStart(0), _selectHeld(false),
    _selState(HIGH), _upState(HIGH), _dwState(HIGH),
    _lastSelState(HIGH), _lastUpState(HIGH), _lastDwState(HIGH) {}

void Navigation::begin() {
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
}

void Navigation::update() {
    readButtons();
    checkResetMode();
    _lastAction = decodeAction();
}

void Navigation::readButtons() {
    _lastSelState = _selState;
    _lastUpState = _upState;
    _lastDwState = _dwState;
    
    _selState = digitalRead(SEL_BTN);
    _upState = digitalRead(UP_BTN);
    _dwState = digitalRead(DW_BTN);
    
    // Debounce
    if (_selState != _lastSelState || _upState != _lastUpState || _dwState != _lastDwState) {
        _lastDebounceTime = millis();
    }
    
    if ((millis() - _lastDebounceTime) > 50) {
        // Stable state
    }
}

NavigationAction Navigation::decodeAction() {
    bool selPressed = (_selState == LOW);
    bool upPressed = (_upState == LOW);
    bool dwPressed = (_dwState == LOW);
    
    if (_mode == MODE_DEFAULT) {
        // 3-button navigation: SELECT + UP/DOWN
        if (selPressed && upPressed) return NAV_UP;
        if (selPressed && dwPressed) return NAV_DW;
        if (selPressed && !upPressed && !dwPressed) return NAV_SEL;

    } else if (_mode == MODE_ENCODER) {
        // Encoder mode: Single button actions
        if (selPressed && !_lastSelState) return NAV_SEL;
        if (upPressed && !_lastUpState) return NAV_UP;
        if (dwPressed && !_lastDwState) return NAV_DW;
        // Long press SELECT for BACK
        if (selPressed && _selectHeld) return NAV_BACK;
    }
    
    return NAV_NONE;
}

NavigationAction Navigation::getAction() {
    return _lastAction;
}

bool Navigation::isPressed(NavigationAction action) {
    return (_lastAction == action);
}

void Navigation::setMode(NavigationMode mode) {
    _mode = mode;
}

NavigationMode Navigation::getMode() {
    return _mode;
}

void Navigation::checkResetMode() {
    if (_mode == MODE_ENCODER && digitalRead(SEL_BTN) == LOW) {
        if (_selectPressStart == 0) {
            _selectPressStart = millis();
        } else if (millis() - _selectPressStart > 3000) {
            // Reset to default mode after 3 seconds
            _mode = MODE_DEFAULT;
            _selectPressStart = 0;
            device_play_tone(2000, 100);
            Serial.println("Navigation reset to default mode");
        }
        _selectHeld = true;
    } else {
        _selectPressStart = 0;
        _selectHeld = false;
    }
}
