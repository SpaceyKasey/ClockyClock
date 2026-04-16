#include "app_state.h"
#include <string.h>

AppState g_state;

void initAppState() {
    uint8_t* fb = g_state.framebuffer;  // preserve framebuffer allocation
    memset(&g_state, 0, sizeof(AppState));
    g_state.framebuffer = fb;

    g_state.currentScreen = SCREEN_CLOCK;
    g_state.previousScreen = SCREEN_CLOCK;

    // Default config: New York, London, Berlin, Sydney
    g_state.config.selectedCities[0] = 0;  // New York
    g_state.config.selectedCities[1] = 4;  // London
    g_state.config.selectedCities[2] = 5;  // Berlin
    g_state.config.selectedCities[3] = 7;  // Sydney
    g_state.config.numCities = 4;
    g_state.config.useFahrenheit = true;

    strcpy(g_state.config.wifiSsid, "");
    strcpy(g_state.config.wifiPassword, "");

    // Initialize city data
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        g_state.cities[i].presetIndex = g_state.config.selectedCities[i];
        g_state.cities[i].weatherValid = false;
        g_state.cities[i].currentWeatherCode = -1;
    }

    g_state.needsFullRedraw = true;

    // Neko starts awake
    g_state.nekoState = 0;  // NEKO_AWAKE
    g_state.nekoStateTimer = 12;
    g_state.nekoCurrentSprite = 0;
    g_state.nekoSleepToggle = false;
}
