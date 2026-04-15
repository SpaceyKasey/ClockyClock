#include "cities.h"
#include <string.h>

CityPreset PRESET_CITIES[MAX_PRESETS];
uint8_t PRESET_COUNT = 0;

struct DefaultCity {
    const char* name;
    const char* posixTz;
    float latitude;
    float longitude;
};

static const DefaultCity DEFAULTS[] = {
    {"New York",     "EST5EDT,M3.2.0,M11.1.0",           40.7128f,  -74.0060f},
    {"Los Angeles",  "PST8PDT,M3.2.0,M11.1.0",          34.0522f, -118.2437f},
    {"Chicago",      "CST6CDT,M3.2.0,M11.1.0",          41.8781f,  -87.6298f},
    {"Denver",       "MST7MDT,M3.2.0,M11.1.0",          39.7392f, -104.9903f},
    {"London",       "GMT0BST,M3.5.0/1,M10.5.0",        51.5074f,   -0.1278f},
    {"Berlin",       "CET-1CEST,M3.5.0,M10.5.0/3",      52.5200f,   13.4050f},
    {"Paris",        "CET-1CEST,M3.5.0,M10.5.0/3",      48.8566f,    2.3522f},
    {"Sydney",       "AEST-10AEDT,M10.1.0,M4.1.0/3",   -33.8688f,  151.2093f},
    {"Melbourne",    "AEST-10AEDT,M10.1.0,M4.1.0/3",   -37.8136f,  144.9631f},
    {"Tokyo",        "JST-9",                            35.6762f,  139.6503f},
    {"Dubai",        "<+04>-4",                          25.2048f,   55.2708f},
    {"Singapore",    "<+08>-8",                           1.3521f,  103.8198f},
    {"Mumbai",       "IST-5:30",                         19.0760f,   72.8777f},
    {"Hong Kong",    "HKT-8",                            22.3193f,  114.1694f},
    {"Auckland",     "NZST-12NZDT,M9.5.0,M4.1.0/3",   -36.8485f,  174.7633f},
    {"Honolulu",     "HST10",                            21.3069f, -157.8583f},
};

static const uint8_t DEFAULT_COUNT = sizeof(DEFAULTS) / sizeof(DEFAULTS[0]);

void citiesLoadDefaults() {
    PRESET_COUNT = DEFAULT_COUNT;
    if (PRESET_COUNT > MAX_PRESETS) PRESET_COUNT = MAX_PRESETS;
    for (uint8_t i = 0; i < PRESET_COUNT; i++) {
        strlcpy(PRESET_CITIES[i].name, DEFAULTS[i].name, CITY_NAME_LEN);
        strlcpy(PRESET_CITIES[i].posixTz, DEFAULTS[i].posixTz, CITY_TZ_LEN);
        PRESET_CITIES[i].latitude = DEFAULTS[i].latitude;
        PRESET_CITIES[i].longitude = DEFAULTS[i].longitude;
    }
}
