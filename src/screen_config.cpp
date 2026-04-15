#include "screen_config.h"
#include "display_common.h"
#include "config_manager.h"
#include "touch_handler.h"
#include "cities.h"
#include <string.h>
#include <stdio.h>

// Max touch zones: back + 2 edit btns + MAX_PRESETS city checkboxes + unit toggle + save + load
static TouchZone configZones[MAX_PRESETS + 8];
static uint8_t configZoneCount = 0;

static const char* statusMessage = nullptr;

// Check if a city index is currently selected
static bool isCitySelected(uint8_t presetIdx) {
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        if (g_state.config.selectedCities[i] == presetIdx) return true;
    }
    return false;
}

// Toggle city selection
static void toggleCity(uint8_t presetIdx) {
    // Check if already selected
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        if (g_state.config.selectedCities[i] == presetIdx) {
            // Remove it (shift remaining)
            if (g_state.config.numCities > 1) {
                for (uint8_t j = i; j < g_state.config.numCities - 1; j++) {
                    g_state.config.selectedCities[j] = g_state.config.selectedCities[j + 1];
                }
                g_state.config.numCities--;
            }
            return;
        }
    }
    // Add it (if room)
    if (g_state.config.numCities < 8) {
        g_state.config.selectedCities[g_state.config.numCities] = presetIdx;
        g_state.config.numCities++;
    }
}

void screenConfigRender() {
    clearFramebuffer();
    configZoneCount = 0;

    // Header
    drawButton(10, 5, 80, 40, "< Back");
    configZones[configZoneCount++] = {10, 5, 80, 40, ZONE_BACK_BTN, 0};

    drawText(FONT_MD, "Configuration", 110, 38, COLOR_BLACK);

    // WiFi section
    int32_t y = 60;
    drawHLine(10, y - 5, SCREEN_W - 20, COLOR_LIGHT);

    // SSID
    char ssidDisplay[80];
    snprintf(ssidDisplay, sizeof(ssidDisplay), "WiFi SSID: %s",
             strlen(g_state.config.wifiSsid) > 0 ? g_state.config.wifiSsid : "(not set)");
    drawText(FONT_MD, ssidDisplay, 20, y + 30, COLOR_BLACK);
    drawButton(SCREEN_W - 100, y + 5, 80, 35, "Edit");
    configZones[configZoneCount++] = {
        (int16_t)(SCREEN_W - 100), (int16_t)(y + 5), 80, 35,
        ZONE_WIFI_SSID_EDIT, 0
    };

    // Password
    y += 45;
    char passDisplay[80];
    if (strlen(g_state.config.wifiPassword) > 0) {
        snprintf(passDisplay, sizeof(passDisplay), "WiFi Pass: ********");
    } else {
        snprintf(passDisplay, sizeof(passDisplay), "WiFi Pass: (not set)");
    }
    drawText(FONT_MD, passDisplay, 20, y + 30, COLOR_BLACK);
    drawButton(SCREEN_W - 100, y + 5, 80, 35, "Edit");
    configZones[configZoneCount++] = {
        (int16_t)(SCREEN_W - 100), (int16_t)(y + 5), 80, 35,
        ZONE_WIFI_PASS_EDIT, 0
    };

    // Locations section
    y = 160;
    drawHLine(10, y - 5, SCREEN_W - 20, COLOR_LIGHT);
    drawText(FONT_SM, "Locations (tap to toggle, max 8):", 20, y + 20, COLOR_MID);
    y += 30;

    // 3-column grid of cities
    int32_t colW = (SCREEN_W - 40) / 3;  // ~306px per column

    for (uint8_t i = 0; i < PRESET_COUNT; i++) {
        uint8_t col = i % 3;
        uint8_t row = i / 3;
        int32_t cx = 20 + col * colW;
        int32_t cy = y + row * 40;

        bool selected = isCitySelected(i);
        drawCheckbox(cx, cy, selected);
        drawText(FONT_SM, PRESET_CITIES[i].name, cx + 30, cy + 20, COLOR_BLACK);

        configZones[configZoneCount++] = {
            (int16_t)cx, (int16_t)cy, (int16_t)(colW - 10), 36,
            ZONE_CITY_TOGGLE, i
        };
    }

    // Bottom controls
    y = 420;
    drawHLine(10, y - 5, SCREEN_W - 20, COLOR_LIGHT);

    // Temperature unit toggle
    bool fahr = g_state.config.useFahrenheit;
    drawText(FONT_MD, "Unit:", 20, y + 35, COLOR_BLACK);
    drawButton(100, y + 8, 50, 35, "F", fahr);
    drawButton(160, y + 8, 50, 35, "C", !fahr);
    configZones[configZoneCount++] = {100, (int16_t)(y + 8), 110, 35, ZONE_UNIT_TOGGLE, 0};

    // Save / Load buttons
    drawButton(400, y + 8, 130, 35, "Save to SD");
    configZones[configZoneCount++] = {400, (int16_t)(y + 8), 130, 35, ZONE_SAVE_BTN, 0};

    drawButton(550, y + 8, 140, 35, "Load from SD");
    configZones[configZoneCount++] = {550, (int16_t)(y + 8), 140, 35, ZONE_LOAD_BTN, 0};

    // Status message
    if (statusMessage) {
        drawText(FONT_SM, statusMessage, 20, SCREEN_H - 25, COLOR_MID);
    }

    // SD card status
    char sdStatus[32];
    snprintf(sdStatus, sizeof(sdStatus), "SD: %s", g_state.sdCardPresent ? "Ready" : "Not found");
    drawTextRight(FONT_SM, sdStatus, SCREEN_W - 20, SCREEN_H - 25, COLOR_MID);

    pushFramebuffer();
}

void screenConfigHandleTouch(int16_t x, int16_t y) {
    const TouchZone* zone = hitTest(x, y, configZones, configZoneCount);
    if (!zone) return;

    switch (zone->id) {
        case ZONE_BACK_BTN:
            configApply();
            g_state.currentScreen = SCREEN_CLOCK;
            g_state.needsFullRedraw = true;
            statusMessage = nullptr;
            break;

        case ZONE_WIFI_SSID_EDIT:
            strncpy(g_state.inputBuffer, g_state.config.wifiSsid, sizeof(g_state.inputBuffer));
            g_state.inputCursor = strlen(g_state.inputBuffer);
            g_state.inputField = 0;
            g_state.shiftActive = false;
            g_state.symbolsActive = false;
            g_state.previousScreen = SCREEN_CONFIG;
            g_state.currentScreen = SCREEN_KEYBOARD;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_WIFI_PASS_EDIT:
            strncpy(g_state.inputBuffer, g_state.config.wifiPassword, sizeof(g_state.inputBuffer));
            g_state.inputCursor = strlen(g_state.inputBuffer);
            g_state.inputField = 1;
            g_state.shiftActive = false;
            g_state.symbolsActive = false;
            g_state.previousScreen = SCREEN_CONFIG;
            g_state.currentScreen = SCREEN_KEYBOARD;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_CITY_TOGGLE:
            toggleCity(zone->data);
            g_state.needsFullRedraw = true;
            statusMessage = nullptr;
            break;

        case ZONE_UNIT_TOGGLE:
            g_state.config.useFahrenheit = !g_state.config.useFahrenheit;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_SAVE_BTN:
            if (configSave()) {
                statusMessage = "Config saved to SD card!";
            } else {
                statusMessage = "Save failed - check SD card";
            }
            g_state.needsFullRedraw = true;
            break;

        case ZONE_LOAD_BTN:
            if (configLoad()) {
                statusMessage = "Config loaded from SD card!";
            } else {
                statusMessage = "Load failed - check SD card";
            }
            g_state.needsFullRedraw = true;
            break;
    }
}
