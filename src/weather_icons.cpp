#include "weather_icons.h"
#include "epd_driver.h"
#include <string.h>
#include <math.h>

// 48x48 icons at 4bpp = 24 bytes per row, 48 rows = 1152 bytes each
// These are procedurally drawn into a static buffer instead of stored as bitmaps
// to save flash space. We draw them once into a scratch buffer.

static uint8_t iconBuffer[ICON_SIZE * ICON_SIZE / 2];

static void clearIcon() {
    memset(iconBuffer, 0xFF, sizeof(iconBuffer));
}

static void setPixel(int x, int y, uint8_t gray) {
    if (x < 0 || x >= ICON_SIZE || y < 0 || y >= ICON_SIZE) return;
    int idx = y * (ICON_SIZE / 2) + x / 2;
    if (x % 2 == 0) {
        iconBuffer[idx] = (iconBuffer[idx] & 0x0F) | ((gray & 0xF0));
    } else {
        iconBuffer[idx] = (iconBuffer[idx] & 0xF0) | ((gray >> 4) & 0x0F);
    }
}

static void drawCircleFilled(int cx, int cy, int r, uint8_t gray) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                setPixel(cx + dx, cy + dy, gray);
            }
        }
    }
}

static void drawLine(int x0, int y0, int x1, int y1, uint8_t gray) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        setPixel(x0, y0, gray);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Sun icon: circle with rays
static void drawSun() {
    clearIcon();
    drawCircleFilled(24, 24, 10, 0x00);
    // Rays
    for (int angle = 0; angle < 8; angle++) {
        float a = angle * 3.14159f / 4.0f;
        int x1 = 24 + (int)(14 * cosf(a));
        int y1 = 24 + (int)(14 * sinf(a));
        int x2 = 24 + (int)(20 * cosf(a));
        int y2 = 24 + (int)(20 * sinf(a));
        drawLine(x1, y1, x2, y2, 0x00);
    }
}

// Cloud icon
static void drawCloud() {
    clearIcon();
    drawCircleFilled(18, 28, 10, 0x40);
    drawCircleFilled(30, 24, 12, 0x40);
    drawCircleFilled(38, 30, 8, 0x40);
    // Flat bottom
    for (int x = 8; x < 47; x++) {
        for (int y = 34; y < 40; y++) {
            setPixel(x, y, 0x40);
        }
    }
}

// Sun + cloud (partly cloudy)
static void drawPartlyCloudy() {
    clearIcon();
    // Sun in upper-left
    drawCircleFilled(16, 16, 8, 0x00);
    for (int angle = 0; angle < 8; angle++) {
        float a = angle * 3.14159f / 4.0f;
        int x1 = 16 + (int)(11 * cosf(a));
        int y1 = 16 + (int)(11 * sinf(a));
        int x2 = 16 + (int)(15 * cosf(a));
        int y2 = 16 + (int)(15 * sinf(a));
        drawLine(x1, y1, x2, y2, 0x00);
    }
    // Cloud in lower-right
    drawCircleFilled(26, 32, 8, 0x40);
    drawCircleFilled(36, 28, 10, 0x40);
    drawCircleFilled(42, 33, 6, 0x40);
    for (int x = 18; x < 48; x++) {
        for (int y = 36; y < 42; y++) {
            setPixel(x, y, 0x40);
        }
    }
}

// Rain icon
static void drawRain() {
    clearIcon();
    // Cloud
    drawCircleFilled(16, 16, 8, 0x40);
    drawCircleFilled(28, 12, 10, 0x40);
    drawCircleFilled(36, 18, 7, 0x40);
    for (int x = 8; x < 44; x++) {
        for (int y = 22; y < 26; y++) {
            setPixel(x, y, 0x40);
        }
    }
    // Rain drops
    drawLine(14, 30, 12, 38, 0x00);
    drawLine(22, 30, 20, 38, 0x00);
    drawLine(30, 30, 28, 38, 0x00);
    drawLine(38, 30, 36, 38, 0x00);
}

// Snow icon
static void drawSnow() {
    clearIcon();
    // Cloud
    drawCircleFilled(16, 14, 8, 0x40);
    drawCircleFilled(28, 10, 10, 0x40);
    drawCircleFilled(36, 16, 7, 0x40);
    for (int x = 8; x < 44; x++) {
        for (int y = 20; y < 24; y++) {
            setPixel(x, y, 0x40);
        }
    }
    // Snowflakes (dots)
    drawCircleFilled(14, 32, 2, 0x00);
    drawCircleFilled(24, 36, 2, 0x00);
    drawCircleFilled(34, 32, 2, 0x00);
    drawCircleFilled(20, 42, 2, 0x00);
    drawCircleFilled(30, 42, 2, 0x00);
}

// Fog icon
static void drawFog() {
    clearIcon();
    for (int i = 0; i < 5; i++) {
        int y = 12 + i * 7;
        int indent = (i % 2 == 0) ? 6 : 10;
        drawLine(indent, y, ICON_SIZE - indent, y, 0x60);
        drawLine(indent, y + 1, ICON_SIZE - indent, y + 1, 0x60);
    }
}

// Thunderstorm icon
static void drawThunderstorm() {
    clearIcon();
    // Dark cloud
    drawCircleFilled(16, 14, 8, 0x20);
    drawCircleFilled(28, 10, 10, 0x20);
    drawCircleFilled(36, 16, 7, 0x20);
    for (int x = 8; x < 44; x++) {
        for (int y = 20; y < 24; y++) {
            setPixel(x, y, 0x20);
        }
    }
    // Lightning bolt
    drawLine(26, 26, 22, 34, 0x00);
    drawLine(22, 34, 28, 34, 0x00);
    drawLine(28, 34, 24, 44, 0x00);
}

// Drizzle icon (light rain)
static void drawDrizzle() {
    clearIcon();
    // Light cloud
    drawCircleFilled(16, 16, 8, 0x80);
    drawCircleFilled(28, 12, 10, 0x80);
    drawCircleFilled(36, 18, 7, 0x80);
    for (int x = 8; x < 44; x++) {
        for (int y = 22; y < 26; y++) {
            setPixel(x, y, 0x80);
        }
    }
    // Small dots for drizzle
    drawCircleFilled(16, 32, 1, 0x40);
    drawCircleFilled(24, 36, 1, 0x40);
    drawCircleFilled(32, 32, 1, 0x40);
    drawCircleFilled(20, 40, 1, 0x40);
    drawCircleFilled(28, 40, 1, 0x40);
}

// Unknown / no data
static void drawUnknown() {
    clearIcon();
    drawCircleFilled(24, 20, 12, 0xA0);
    // Question mark - just draw a ?
    drawLine(20, 14, 24, 10, 0x00);
    drawLine(24, 10, 28, 14, 0x00);
    drawLine(28, 14, 24, 20, 0x00);
    drawCircleFilled(24, 26, 2, 0x00);
}

const uint8_t* getWeatherIcon(int wmoCode) {
    if (wmoCode == 0) {
        drawSun();
    } else if (wmoCode >= 1 && wmoCode <= 3) {
        drawPartlyCloudy();
    } else if (wmoCode == 45 || wmoCode == 48) {
        drawFog();
    } else if (wmoCode >= 51 && wmoCode <= 57) {
        drawDrizzle();
    } else if (wmoCode >= 61 && wmoCode <= 67) {
        drawRain();
    } else if (wmoCode >= 71 && wmoCode <= 77) {
        drawSnow();
    } else if (wmoCode >= 80 && wmoCode <= 82) {
        drawRain();
    } else if (wmoCode >= 85 && wmoCode <= 86) {
        drawSnow();
    } else if (wmoCode >= 95 && wmoCode <= 99) {
        drawThunderstorm();
    } else {
        drawUnknown();
        if (wmoCode < 0) return nullptr;
    }
    return iconBuffer;
}

const char* getWeatherDescription(int wmoCode) {
    switch (wmoCode) {
        case 0:  return "Clear";
        case 1:  return "Mostly Clear";
        case 2:  return "Partly Cloudy";
        case 3:  return "Overcast";
        case 45: return "Fog";
        case 48: return "Rime Fog";
        case 51: return "Light Drizzle";
        case 53: return "Drizzle";
        case 55: return "Heavy Drizzle";
        case 56: return "Freezing Drizzle";
        case 57: return "Heavy Frz Drizzle";
        case 61: return "Light Rain";
        case 63: return "Rain";
        case 65: return "Heavy Rain";
        case 66: return "Freezing Rain";
        case 67: return "Heavy Frz Rain";
        case 71: return "Light Snow";
        case 73: return "Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Light Showers";
        case 81: return "Showers";
        case 82: return "Heavy Showers";
        case 85: return "Light Snow Shwrs";
        case 86: return "Heavy Snow Shwrs";
        case 95: return "Thunderstorm";
        case 96: return "T-storm w/ Hail";
        case 99: return "T-storm w/ Hvy Hail";
        default: return "Unknown";
    }
}
