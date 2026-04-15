#include "screen_forecast.h"
#include "display_common.h"
#include "weather_icons.h"
#include "time_utils.h"
#include "touch_handler.h"
#include "cities.h"
#include <stdio.h>
#include <string.h>

static TouchZone forecastZones[1];

void screenForecastRender() {
    clearFramebuffer();

    uint8_t ci = g_state.selectedCityIndex;
    const CityData& city = g_state.cities[ci];
    const CityPreset& preset = PRESET_CITIES[city.presetIndex];

    // Header: Back button + city name
    drawButton(10, 5, 80, 40, "< Back");
    forecastZones[0] = {10, 5, 80, 40, ZONE_BACK_BTN, 0};

    char title[64];
    snprintf(title, sizeof(title), "%s - 7 Day Forecast", preset.name);
    drawText(FONT_MD, title, 110, 38, COLOR_BLACK);

    // Current conditions
    drawHLine(10, HEADER_H, SCREEN_W - 20, COLOR_LIGHT);

    if (city.weatherValid) {
        char currentBuf[64];
        snprintf(currentBuf, sizeof(currentBuf), "Now: %.0f%s  %s",
                 city.currentTemp,
                 g_state.config.useFahrenheit ? "F" : "C",
                 getWeatherDescription(city.currentWeatherCode));
        drawText(FONT_MD, currentBuf, 30, HEADER_H + 50, COLOR_BLACK);
        drawWeatherIcon(SCREEN_W - 100, HEADER_H + 15, city.currentWeatherCode);
    }

    // 7-day forecast columns
    int32_t forecastY = 130;
    drawHLine(10, forecastY, SCREEN_W - 20, COLOR_LIGHT);

    int32_t colW = (SCREEN_W - 40) / 7;  // ~131px per column

    // Get day names starting from today
    struct tm t = city.localTime;

    for (int day = 0; day < 7; day++) {
        int32_t colX = 20 + day * colW;
        int32_t centerX = colX + colW / 2;

        // Day name
        char dayName[8];
        if (day == 0) {
            strcpy(dayName, "Today");
        } else {
            struct tm futureDay = t;
            futureDay.tm_mday += day;
            mktime(&futureDay);  // normalize
            formatDayShort(&futureDay, dayName, sizeof(dayName));
        }
        drawTextCentered(FONT_SM, dayName, centerX, forecastY + 30, COLOR_BLACK);

        // Weather icon
        if (city.forecast[day].code >= 0) {
            drawWeatherIcon(centerX - ICON_SIZE / 2, forecastY + 40, city.forecast[day].code);

            // High temp
            char tempBuf[16];
            snprintf(tempBuf, sizeof(tempBuf), "%.0f%s",
                     city.forecast[day].tempMax,
                     g_state.config.useFahrenheit ? "F" : "C");
            drawTextCentered(FONT_MD, tempBuf, centerX, forecastY + 140, COLOR_BLACK);

            // Low temp
            snprintf(tempBuf, sizeof(tempBuf), "%.0f%s",
                     city.forecast[day].tempMin,
                     g_state.config.useFahrenheit ? "F" : "C");
            drawTextCentered(FONT_SM, tempBuf, centerX, forecastY + 175, COLOR_MID);
        }

        // Column separator (except last)
        if (day < 6) {
            epd_draw_vline(colX + colW, forecastY + 10, 190, COLOR_LIGHT, g_state.framebuffer);
        }
    }

    // Sunrise / Sunset info at bottom
    int32_t infoY = 400;
    drawHLine(10, infoY, SCREEN_W - 20, COLOR_LIGHT);

    if (city.weatherValid) {
        char srBuf[32], ssBuf[32];
        snprintf(srBuf, sizeof(srBuf), "Sunrise: %s", city.sunrise);
        snprintf(ssBuf, sizeof(ssBuf), "Sunset: %s", city.sunset);
        drawText(FONT_MD, srBuf, 60, infoY + 50, COLOR_MID);
        drawText(FONT_MD, ssBuf, 400, infoY + 50, COLOR_MID);

        // Local time
        char timeBuf[32];
        formatTime12h(&city.localTime, timeBuf, sizeof(timeBuf));
        char localBuf[64];
        snprintf(localBuf, sizeof(localBuf), "Local time: %s", timeBuf);
        drawTextRight(FONT_MD, localBuf, SCREEN_W - 30, infoY + 50, COLOR_MID);
    }

    pushFramebuffer();
}

void screenForecastHandleTouch(int16_t x, int16_t y) {
    const TouchZone* zone = hitTest(x, y, forecastZones, 1);
    if (zone && zone->id == ZONE_BACK_BTN) {
        g_state.currentScreen = SCREEN_CLOCK;
        g_state.needsFullRedraw = true;
    }
}
