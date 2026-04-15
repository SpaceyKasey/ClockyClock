#pragma once
#include <stdint.h>

#define MAX_PRESETS     24
#define CITY_NAME_LEN   32
#define CITY_TZ_LEN     48

struct CityPreset {
    char name[CITY_NAME_LEN];
    char posixTz[CITY_TZ_LEN];
    float latitude;
    float longitude;
};

extern CityPreset PRESET_CITIES[MAX_PRESETS];
extern uint8_t PRESET_COUNT;

// Populate PRESET_CITIES with hardcoded defaults
void citiesLoadDefaults();
