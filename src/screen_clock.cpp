#include "screen_clock.h"
#include "display_common.h"
#include "time_utils.h"
#include "touch_handler.h"
#include "cities.h"
#include "neko_sprites.h"
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

// Convert wind direction degrees to compass string
static const char* windDirStr(int deg) {
    static const char* dirs[] = {"N","NNE","NE","ENE","E","ESE","SE","SSE",
                                  "S","SSW","SW","WSW","W","WNW","NW","NNW"};
    return dirs[((deg + 11) / 22) % 16];
}

// Parse "HH:MM" string to minutes since midnight
static int parseTimeToMinutes(const char* hhmm) {
    if (!hhmm || strlen(hhmm) < 4) return -1;
    int h = atoi(hhmm);
    const char* colon = strchr(hhmm, ':');
    if (!colon) return -1;
    int m = atoi(colon + 1);
    return h * 60 + m;
}

static bool isNightTime(const CityData& city) {
    if (!city.weatherValid || city.sunrise[0] == '\0') return false;
    int nowMin = city.localTime.tm_hour * 60 + city.localTime.tm_min;
    int rise = parseTimeToMinutes(city.sunrise);
    int set  = parseTimeToMinutes(city.sunset);
    if (rise < 0 || set < 0) return false;
    return nowMin < rise || nowMin >= set;
}

static void drawCityRow(uint8_t rowIdx, uint8_t cityIdx) {
    const CityData& city = g_state.cities[cityIdx];
    const CityPreset& preset = PRESET_CITIES[city.presetIndex];

    int32_t y = HEADER_H + rowIdx * ROW_H;

    // Check if nighttime for color inversion
    bool night = g_state.ntpSynced && isNightTime(city);

    // Background fill for night rows
    if (night) {
        epd_fill_rect(0, y + 1, SCREEN_W, ROW_H - 1, COLOR_BLACK, g_state.framebuffer);
    }

    // Colors based on day/night
    uint8_t fg    = night ? COLOR_WHITE : COLOR_BLACK;
    uint8_t fg2   = night ? COLOR_WHITE : COLOR_MID;
    uint8_t bg    = night ? COLOR_BLACK : COLOR_WHITE;

    // Separator line
    drawHLine(10, y, SCREEN_W - 20, night ? COLOR_DARK : COLOR_LIGHT);

    // Layout columns
    int32_t nameX = 20;
    int32_t timeX = 245;
    int32_t dateX = 585;
    int32_t todayWx = 690;
    int32_t tmrwWx = 820;

    // City name
    drawText(FONT_MD, preset.name, nameX, y + 42, fg, bg);

    // Sunrise/sunset under city name
    if (city.weatherValid && city.sunrise[0] != '\0') {
        char sunBuf[20];
        snprintf(sunBuf, sizeof(sunBuf), "%s-%s", city.sunrise, city.sunset);
        drawText(FONT_SM, sunBuf, nameX, y + 72, fg2, bg);
    }

    // Day of week + date under sunrise/sunset
    if (g_state.ntpSynced) {
        char dateBuf[16];
        formatDate(&city.localTime, dateBuf, sizeof(dateBuf));
        drawText(FONT_SM, dateBuf, nameX, y + 96, fg2, bg);
    }

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
    drawText(FONT_LG, timeBuf, timeX, y + 75, fg, bg);

    // Wind + humidity (where date column used to be)
    if (city.weatherValid) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%.0f%s",
                 city.currentWindSpeed,
                 g_state.config.useFahrenheit ? "mph" : "km/h");
        drawText(FONT_SM, buf, dateX, y + 35, fg2, bg);
        drawText(FONT_SM, windDirStr(city.currentWindDir), dateX, y + 60, fg2, bg);
        snprintf(buf, sizeof(buf), "%d%%rh", city.currentHumidity);
        drawText(FONT_SM, buf, dateX, y + 85, fg2, bg);
    }

    // Weather
    if (city.weatherValid) {
        const char* unit = g_state.config.useFahrenheit ? "F" : "C";
        char tempBuf[16];

        // Today: icon + current temp, H/L, precip
        drawWeatherIcon(todayWx, y + 5, city.currentWeatherCode, night);
        snprintf(tempBuf, sizeof(tempBuf), "%.0f%s", city.currentTemp, unit);
        drawText(FONT_SM, tempBuf, todayWx + 52, y + 35, fg, bg);
        snprintf(tempBuf, sizeof(tempBuf), "%.0f/%.0f",
                 city.forecast[0].tempMax, city.forecast[0].tempMin);
        drawText(FONT_SM, tempBuf, todayWx + 52, y + 60, fg2, bg);
        snprintf(tempBuf, sizeof(tempBuf), "%.0fh %d%%",
                 city.forecast[0].precipHours, city.forecast[0].precipChance);
        drawText(FONT_SM, tempBuf, todayWx + 52, y + 85, fg2, bg);

        // Tomorrow: icon + max temp, H/L, precip
        if (city.forecast[1].code >= 0) {
            drawWeatherIcon(tmrwWx, y + 5, city.forecast[1].code, night);
            snprintf(tempBuf, sizeof(tempBuf), "%.0f%s",
                     city.forecast[1].tempMax, unit);
            drawText(FONT_SM, tempBuf, tmrwWx + 52, y + 35, fg2, bg);
            snprintf(tempBuf, sizeof(tempBuf), "%.0f/%.0f",
                     city.forecast[1].tempMax, city.forecast[1].tempMin);
            drawText(FONT_SM, tempBuf, tmrwWx + 52, y + 60, fg2, bg);
            snprintf(tempBuf, sizeof(tempBuf), "%.0fh %d%%",
                     city.forecast[1].precipHours, city.forecast[1].precipChance);
            drawText(FONT_SM, tempBuf, tmrwWx + 52, y + 85, fg2, bg);
        }
    }
}

void screenClockRender() {
    clearFramebuffer();

    // Header with Neko
    nekoUpdate();
    nekoDrawAt(16, 9);
    drawText(FONT_MD, "CLOCKYCLOCK", 54, 38, COLOR_BLACK);

    // Config gear button
    drawButton(SCREEN_W - 70, 5, 60, 40, "CFG");

    // Subtitle labels
    drawText(FONT_SM, "Wind", 585, 45, COLOR_MID);
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
