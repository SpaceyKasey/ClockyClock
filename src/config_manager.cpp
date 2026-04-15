#include "config_manager.h"
#include "cities.h"
#include "pins.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

static const char* CONFIG_PATH  = "/config.json";
static const char* CITIES_PATH  = "/cities.json";

bool sdCardInit() {
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
    if (!SD.begin(SD_CS)) {
        Serial.println("SD: Mount failed");
        g_state.sdCardPresent = false;
        return false;
    }
    Serial.println("SD: Mounted OK");
    g_state.sdCardPresent = true;
    return true;
}

bool configSave() {
    if (!g_state.sdCardPresent) return false;

    JsonDocument doc;
    doc["wifi_ssid"] = g_state.config.wifiSsid;
    doc["wifi_pass"] = g_state.config.wifiPassword;
    doc["use_fahrenheit"] = g_state.config.useFahrenheit;
    doc["use_24hour"] = g_state.config.use24Hour;

    JsonArray cities = doc["cities"].to<JsonArray>();
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        cities.add(g_state.config.selectedCities[i]);
    }

    // Write to temp file first, then rename for safety
    const char* tmpPath = "/config.tmp";
    File f = SD.open(tmpPath, FILE_WRITE);
    if (!f) {
        Serial.println("SD: Failed to open temp file");
        return false;
    }

    serializeJsonPretty(doc, f);
    f.close();

    // Remove old and rename
    SD.remove(CONFIG_PATH);
    SD.rename(tmpPath, CONFIG_PATH);

    Serial.println("Config: Saved to SD");
    return true;
}

bool configLoad() {
    if (!g_state.sdCardPresent) return false;

    File f = SD.open(CONFIG_PATH, FILE_READ);
    if (!f) {
        Serial.println("Config: No config file on SD");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("Config: JSON parse error: %s\n", err.c_str());
        return false;
    }

    strlcpy(g_state.config.wifiSsid, doc["wifi_ssid"] | "", sizeof(g_state.config.wifiSsid));
    strlcpy(g_state.config.wifiPassword, doc["wifi_pass"] | "", sizeof(g_state.config.wifiPassword));
    g_state.config.useFahrenheit = doc["use_fahrenheit"] | true;
    g_state.config.use24Hour = doc["use_24hour"] | false;

    JsonArray cities = doc["cities"];
    g_state.config.numCities = 0;
    for (JsonVariant v : cities) {
        uint8_t idx = v.as<uint8_t>();
        if (idx < PRESET_COUNT && g_state.config.numCities < 8) {
            g_state.config.selectedCities[g_state.config.numCities] = idx;
            g_state.config.numCities++;
        }
    }

    if (g_state.config.numCities == 0) {
        // Fallback defaults
        g_state.config.selectedCities[0] = 0;
        g_state.config.numCities = 1;
    }

    Serial.printf("Config: Loaded - %d cities, WiFi='%s'\n",
                  g_state.config.numCities, g_state.config.wifiSsid);
    return true;
}

void configApply() {
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        g_state.cities[i].presetIndex = g_state.config.selectedCities[i];
        g_state.cities[i].weatherValid = false;
        g_state.cities[i].currentWeatherCode = -1;
    }
    g_state.needsFullRedraw = true;
}

// Write current PRESET_CITIES to SD as a starter template
static void citiesWriteDefaults() {
    File f = SD.open(CITIES_PATH, FILE_WRITE);
    if (!f) return;

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (uint8_t i = 0; i < PRESET_COUNT; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = PRESET_CITIES[i].name;
        obj["tz"]   = PRESET_CITIES[i].posixTz;
        obj["lat"]  = serialized(String(PRESET_CITIES[i].latitude, 4));
        obj["lon"]  = serialized(String(PRESET_CITIES[i].longitude, 4));
    }

    serializeJsonPretty(doc, f);
    f.close();
    Serial.println("Cities: Wrote defaults to SD");
}

bool citiesLoad() {
    if (!g_state.sdCardPresent) return false;

    File f = SD.open(CITIES_PATH, FILE_READ);
    if (!f) {
        Serial.println("Cities: No cities.json - writing defaults");
        citiesWriteDefaults();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err || !doc.is<JsonArray>()) {
        Serial.printf("Cities: JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    PRESET_COUNT = 0;
    for (JsonVariant v : arr) {
        if (PRESET_COUNT >= MAX_PRESETS) break;
        JsonObject obj = v.as<JsonObject>();
        strlcpy(PRESET_CITIES[PRESET_COUNT].name,
                obj["name"] | "Unknown", CITY_NAME_LEN);
        strlcpy(PRESET_CITIES[PRESET_COUNT].posixTz,
                obj["tz"] | "UTC0", CITY_TZ_LEN);
        PRESET_CITIES[PRESET_COUNT].latitude  = obj["lat"] | 0.0f;
        PRESET_CITIES[PRESET_COUNT].longitude = obj["lon"] | 0.0f;
        PRESET_COUNT++;
    }

    if (PRESET_COUNT == 0) {
        Serial.println("Cities: Empty file - using defaults");
        citiesLoadDefaults();
        return false;
    }

    Serial.printf("Cities: Loaded %d from SD\n", PRESET_COUNT);
    return true;
}

void citiesReset() {
    citiesLoadDefaults();
    if (g_state.sdCardPresent) {
        citiesWriteDefaults();
    }
    Serial.printf("Cities: Reset to %d defaults\n", PRESET_COUNT);
}

void citiesSave() {
    if (!g_state.sdCardPresent) {
        Serial.println("Cities: No SD card");
        return;
    }
    citiesWriteDefaults();  // writes current PRESET_CITIES to SD
}

