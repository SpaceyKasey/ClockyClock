#include "display_common.h"
#include "weather_icons.h"
#include "pins.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

void clearFramebuffer() {
    memset(g_state.framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void pushFramebuffer() {
    epd_poweron();
    delay(10);
    // Read battery while power is on (required for ADC on this board)
    g_state.batteryVoltage = analogReadMilliVolts(BATT_PIN) * 2.0f / 1000.0f;
    if (g_state.needsFullRedraw) {
        // Full clear: multi-cycle flash to fully remove ghosting
        epd_clear();
    } else {
        // Partial: two quick white pushes to clear old pixels
        epd_push_pixels(epd_full_screen(), 50, 1);
        epd_push_pixels(epd_full_screen(), 50, 1);
    }
    epd_draw_grayscale_image(epd_full_screen(), g_state.framebuffer);
    epd_poweroff();
}

void drawText(const GFXfont* font, const char* text, int32_t x, int32_t y, uint8_t color, uint8_t bg) {
    int32_t cx = x, cy = y;
    FontProperties props = {0};
    props.fg_color = color;
    props.bg_color = bg;
    write_mode(font, text, &cx, &cy, g_state.framebuffer, BLACK_ON_WHITE, &props);
}

void drawTextRight(const GFXfont* font, const char* text, int32_t rightX, int32_t y, uint8_t color, uint8_t bg) {
    int32_t w = getTextWidth(font, text);
    drawText(font, text, rightX - w, y, color, bg);
}

void drawTextCentered(const GFXfont* font, const char* text, int32_t centerX, int32_t y, uint8_t color, uint8_t bg) {
    int32_t w = getTextWidth(font, text);
    drawText(font, text, centerX - w / 2, y, color, bg);
}

int32_t getTextWidth(const GFXfont* font, const char* text) {
    int32_t x = 0, y = 0;
    int32_t x1, y1, w, h;
    get_text_bounds(font, text, &x, &y, &x1, &y1, &w, &h, NULL);
    return w;
}

void drawHLine(int32_t x, int32_t y, int32_t w, uint8_t color) {
    epd_draw_hline(x, y, w, color, g_state.framebuffer);
}

void drawButton(int32_t x, int32_t y, int32_t w, int32_t h, const char* label, bool filled) {
    if (filled) {
        epd_fill_rect(x, y, w, h, COLOR_BLACK, g_state.framebuffer);
        drawTextCentered(FONT_SM, label, x + w / 2, y + h / 2 + 10, COLOR_WHITE);
    } else {
        epd_draw_rect(x, y, w, h, COLOR_BLACK, g_state.framebuffer);
        drawTextCentered(FONT_SM, label, x + w / 2, y + h / 2 + 10, COLOR_BLACK);
    }
}

void drawCheckbox(int32_t x, int32_t y, bool checked) {
    epd_draw_rect(x, y, 24, 24, COLOR_BLACK, g_state.framebuffer);
    if (checked) {
        // Draw X inside
        epd_draw_line(x + 4, y + 4, x + 20, y + 20, COLOR_BLACK, g_state.framebuffer);
        epd_draw_line(x + 20, y + 4, x + 4, y + 20, COLOR_BLACK, g_state.framebuffer);
    }
}

static uint8_t invertedIconBuf[ICON_SIZE * ICON_SIZE / 2];

void drawWeatherIcon(int32_t x, int32_t y, int weatherCode, bool invert) {
    const uint8_t* icon = getWeatherIcon(weatherCode);
    if (icon) {
        const uint8_t* src = icon;
        if (invert) {
            for (int i = 0; i < ICON_SIZE * ICON_SIZE / 2; i++) {
                // Each byte has two 4-bit pixels; invert both (15 - val)
                uint8_t hi = (icon[i] >> 4) & 0x0F;
                uint8_t lo = icon[i] & 0x0F;
                invertedIconBuf[i] = ((15 - hi) << 4) | (15 - lo);
            }
            src = invertedIconBuf;
        }
        Rect_t area = {x, y, ICON_SIZE, ICON_SIZE};
        epd_copy_to_framebuffer(area, (uint8_t*)src, g_state.framebuffer);
    }
}

void drawStatusBar() {
    int32_t y = SCREEN_H - STATUS_H;
    drawHLine(0, y, SCREEN_W, COLOR_LIGHT);

    char status[128];
    snprintf(status, sizeof(status), "WiFi: %s | NTP: %s | Weather: %s",
             g_state.wifiConnected ? "OK" : "---",
             g_state.ntpSynced ? "Synced" : "---",
             g_state.cities[0].weatherValid ? "OK" : "---");

    drawText(FONT_SM, status, 20, y + 30, COLOR_MID);

    // Battery indicator (bottom right)
    if (g_state.batteryVoltage > 0.5f) {
        int pct = (int)((g_state.batteryVoltage - 3.0f) / 1.2f * 100.0f);
        if (pct > 100) pct = 100;
        if (pct < 0) pct = 0;
        char battBuf[24];
        snprintf(battBuf, sizeof(battBuf), "%.1fV %d%%", g_state.batteryVoltage, pct);
        drawTextRight(FONT_SM, battBuf, SCREEN_W - 15, y + 30, COLOR_MID);
    }
}
