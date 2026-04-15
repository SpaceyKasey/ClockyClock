#include "screen_clock.h"
#include "display_common.h"
#include "time_utils.h"
#include "touch_handler.h"
#include "cities.h"
#include <stdio.h>
#include <string.h>

// Touch zones for the clock screen
static TouchZone clockZones[6]; // max 4 rows + config btn + page nav
static uint8_t clockZoneCount = 0;

static void buildZones() {
    clockZoneCount = 0;
    uint8_t startIdx = g_state.clockPage * MAX_VISIBLE_ROWS;
    uint8_t visibleCount = g_state.config.numCities - startIdx;
    if (visibleCount > MAX_VISIBLE_ROWS) visibleCount = MAX_VISIBLE_ROWS;

    // City rows
    for (uint8_t i = 0; i < visibleCount; i++) {
        clockZones[clockZoneCount++] = {
            0, (int16_t)(HEADER_H + i * ROW_H),
            SCREEN_W, ROW_H,
            ZONE_CITY_ROW, (uint8_t)(startIdx + i)
        };
    }

    // Config button (top right)
    clockZones[clockZoneCount++] = {
        SCREEN_W - 70, 5, 60, 40,
        ZONE_CONFIG_BTN, 0
    };

    // Page navigation if needed
    if (g_state.config.numCities > MAX_VISIBLE_ROWS) {
        if (g_state.clockPage > 0) {
            clockZones[clockZoneCount++] = {
                SCREEN_W - 140, (int16_t)(SCREEN_H - STATUS_H + 5), 60, 35,
                ZONE_PAGE_PREV, 0
            };
        }
        if (startIdx + MAX_VISIBLE_ROWS < g_state.config.numCities) {
            clockZones[clockZoneCount++] = {
                SCREEN_W - 70, (int16_t)(SCREEN_H - STATUS_H + 5), 60, 35,
                ZONE_PAGE_NEXT, 0
            };
        }
    }
}

static void drawCityRow(uint8_t rowIdx, uint8_t cityIdx) {
    const CityData& city = g_state.cities[cityIdx];
    const CityPreset& preset = PRESET_CITIES[city.presetIndex];

    int32_t y = HEADER_H + rowIdx * ROW_H;

    // Separator line
    drawHLine(10, y, SCREEN_W - 20, COLOR_LIGHT);

    // Layout columns
    int32_t nameX = 20;
    int32_t timeX = 245;
    int32_t dateX = 585;
    int32_t todayWx = 690;
    int32_t tmrwWx = 820;

    int32_t textBaseline = y + 65;    // Vertical center for large text

    // City name (medium font)
    drawText(FONT_MD, preset.name, nameX, y + 55, COLOR_BLACK);

    // Time (large font)
    char timeBuf[16];
    if (g_state.ntpSynced) {
        if (g_state.config.use24Hour)
            formatTime24h(&city.localTime, timeBuf, sizeof(timeBuf));
        else
            formatTime12h(&city.localTime, timeBuf, sizeof(timeBuf));
    } else {
        strcpy(timeBuf, "--:--");
    }
    drawText(FONT_LG, timeBuf, timeX, textBaseline + 10, COLOR_BLACK);

    // Date - stacked: day of week above, month+day below
    if (g_state.ntpSynced) {
        char dayBuf[8], mdBuf[16];
        formatDayShort(&city.localTime, dayBuf, sizeof(dayBuf));
        formatMonthDay(&city.localTime, mdBuf, sizeof(mdBuf));
        drawText(FONT_SM, dayBuf, dateX, y + 40, COLOR_MID);
        drawText(FONT_SM, mdBuf, dateX, y + 70, COLOR_MID);
    } else {
        drawText(FONT_SM, "---", dateX, y + 55, COLOR_MID);
    }

    // Weather
    if (city.weatherValid) {
        const char* unit = g_state.config.useFahrenheit ? "F" : "C";
        char tempBuf[16];

        // Today: icon + current temp, then H/L below
        drawWeatherIcon(todayWx, y + 10, city.currentWeatherCode);
        snprintf(tempBuf, sizeof(tempBuf), "%.0f%s", city.currentTemp, unit);
        drawText(FONT_SM, tempBuf, todayWx + 52, y + 42, COLOR_BLACK);
        snprintf(tempBuf, sizeof(tempBuf), "%.0f/%.0f",
                 city.forecast[0].tempMax, city.forecast[0].tempMin);
        drawText(FONT_SM, tempBuf, todayWx + 52, y + 70, COLOR_MID);

        // Tomorrow: icon + max temp, then H/L below
        if (city.forecast[1].code >= 0) {
            drawWeatherIcon(tmrwWx, y + 10, city.forecast[1].code);
            snprintf(tempBuf, sizeof(tempBuf), "%.0f%s",
                     city.forecast[1].tempMax, unit);
            drawText(FONT_SM, tempBuf, tmrwWx + 52, y + 42, COLOR_MID);
            snprintf(tempBuf, sizeof(tempBuf), "%.0f/%.0f",
                     city.forecast[1].tempMax, city.forecast[1].tempMin);
            drawText(FONT_SM, tempBuf, tmrwWx + 52, y + 70, COLOR_MID);
        }
    }
}

void screenClockRender() {
    clearFramebuffer();

    // Header
    drawText(FONT_MD, "CLOCKYCLOCK", 20, 38, COLOR_BLACK);

    // Config gear button
    drawButton(SCREEN_W - 70, 5, 60, 40, "CFG");

    // Subtitle labels
    drawText(FONT_SM, "Today", 690, 45, COLOR_MID);
    drawText(FONT_SM, "Tmrw", 825, 45, COLOR_MID);

    // City rows
    uint8_t startIdx = g_state.clockPage * MAX_VISIBLE_ROWS;
    uint8_t visibleCount = g_state.config.numCities - startIdx;
    if (visibleCount > MAX_VISIBLE_ROWS) visibleCount = MAX_VISIBLE_ROWS;

    for (uint8_t i = 0; i < visibleCount; i++) {
        drawCityRow(i, startIdx + i);
    }

    // Bottom separator
    drawHLine(10, SCREEN_H - STATUS_H, SCREEN_W - 20, COLOR_LIGHT);

    // Page indicators
    if (g_state.config.numCities > MAX_VISIBLE_ROWS) {
        char pageBuf[16];
        snprintf(pageBuf, sizeof(pageBuf), "Page %d/%d",
                 g_state.clockPage + 1,
                 (g_state.config.numCities + MAX_VISIBLE_ROWS - 1) / MAX_VISIBLE_ROWS);
        drawText(FONT_SM, pageBuf, SCREEN_W - 200, SCREEN_H - 20, COLOR_MID);

        if (g_state.clockPage > 0) {
            drawButton(SCREEN_W - 140, SCREEN_H - STATUS_H + 5, 60, 35, "<");
        }
        uint8_t maxPage = (g_state.config.numCities - 1) / MAX_VISIBLE_ROWS;
        if (g_state.clockPage < maxPage) {
            drawButton(SCREEN_W - 70, SCREEN_H - STATUS_H + 5, 60, 35, ">");
        }
    }

    // Status bar
    drawStatusBar();

    buildZones();
    pushFramebuffer();
}

void screenClockHandleTouch(int16_t x, int16_t y) {
    const TouchZone* zone = hitTest(x, y, clockZones, clockZoneCount);
    if (!zone) return;

    switch (zone->id) {
        case ZONE_CITY_ROW:
            if (g_state.cities[zone->data].weatherValid) {
                g_state.selectedCityIndex = zone->data;
                g_state.previousScreen = SCREEN_CLOCK;
                g_state.currentScreen = SCREEN_FORECAST;
                g_state.needsFullRedraw = true;
            }
            break;

        case ZONE_CONFIG_BTN:
            g_state.previousScreen = SCREEN_CLOCK;
            g_state.currentScreen = SCREEN_CONFIG;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_PAGE_NEXT: {
            uint8_t maxPage = (g_state.config.numCities - 1) / MAX_VISIBLE_ROWS;
            if (g_state.clockPage < maxPage) {
                g_state.clockPage++;
                g_state.needsFullRedraw = true;
            }
            break;
        }

        case ZONE_PAGE_PREV:
            if (g_state.clockPage > 0) {
                g_state.clockPage--;
                g_state.needsFullRedraw = true;
            }
            break;
    }
}
