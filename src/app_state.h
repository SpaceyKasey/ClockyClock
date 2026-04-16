#pragma once
#include <stdint.h>
#include <time.h>
#include "cities.h"
#include "epd_driver.h"

// --- Screen enumeration ---
enum Screen {
    SCREEN_CLOCK,
    SCREEN_FORECAST,
    SCREEN_CONFIG,
    SCREEN_KEYBOARD,
};

// --- Weather data ---
struct WeatherDay {
    int code;           // WMO weather code
    float tempMax;
    float tempMin;
    float precipHours;  // hours of precipitation
    int precipChance;   // max probability % (0-100)
};

struct CityData {
    uint8_t presetIndex;
    struct tm localTime;
    float currentTemp;
    float currentWindSpeed;
    int currentWindDir;
    int currentHumidity;
    int currentWeatherCode;
    WeatherDay forecast[7];
    char sunrise[6];    // "06:23"
    char sunset[6];     // "19:45"
    bool weatherValid;
    unsigned long weatherLastUpdate;
};

// --- Configuration (persisted to SD) ---
struct AppConfig {
    char wifiSsid[64];
    char wifiPassword[64];
    uint8_t selectedCities[8];  // indices into PRESET_CITIES[]
    uint8_t numCities;          // 1-8
    bool useFahrenheit;
    bool use24Hour;
};

// --- Touch zone ---
struct TouchZone {
    int16_t x, y, w, h;
    uint8_t id;
    uint8_t data;       // extra payload (e.g. city index)
};

// Touch zone IDs
enum ZoneId : uint8_t {
    ZONE_NONE = 0,
    ZONE_CITY_ROW,      // data = row index
    ZONE_CONFIG_BTN,
    ZONE_BACK_BTN,
    ZONE_CITY_TOGGLE,   // data = preset index
    ZONE_WIFI_SSID_EDIT,
    ZONE_WIFI_PASS_EDIT,
    ZONE_UNIT_TOGGLE,
    ZONE_SAVE_BTN,
    ZONE_LOAD_BTN,
    ZONE_KEY,           // data = key character
    ZONE_SHIFT_KEY,
    ZONE_BACKSPACE_KEY,
    ZONE_DONE_KEY,
    ZONE_CANCEL_KEY,
    ZONE_SPACE_KEY,
    ZONE_SYMBOLS_KEY,
    ZONE_PAGE_NEXT,
    ZONE_PAGE_PREV,
};

// --- Application state ---
struct AppState {
    Screen currentScreen;
    Screen previousScreen;
    uint8_t selectedCityIndex;  // which city row was tapped (for forecast)

    AppConfig config;
    CityData cities[8];

    bool wifiConnected;
    bool ntpSynced;
    bool sdCardPresent;

    uint8_t* framebuffer;

    // Clock screen pagination (max 4 rows visible)
    uint8_t clockPage;

    // Keyboard state
    char inputBuffer[64];
    uint8_t inputCursor;
    uint8_t inputField;     // 0=SSID, 1=password
    bool shiftActive;
    bool symbolsActive;

    // Refresh tracking
    uint8_t partialUpdateCount;
    unsigned long lastDisplayUpdate;
    unsigned long lastWeatherFetch;
    unsigned long lastNtpSync;

    // Dirty flag - screen needs redraw
    bool needsFullRedraw;
    bool needsPartialUpdate;

    // Battery
    float batteryVoltage;

};

// Global state
extern AppState g_state;

void initAppState();
