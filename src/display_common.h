#pragma once
#include "app_state.h"
#include "fonts.h"

// Colors (4-bit grayscale: 0=black, 15=white)
#define COLOR_BLACK     0
#define COLOR_DARK      4
#define COLOR_MID       8
#define COLOR_LIGHT     12
#define COLOR_WHITE     15

// Layout constants
#define HEADER_H        50
#define ROW_H           110
#define STATUS_H        50
#define MAX_VISIBLE_ROWS 4

#define SCREEN_W EPD_WIDTH   // 960
#define SCREEN_H EPD_HEIGHT  // 540

void clearFramebuffer();
void pushFramebuffer();

// Text helpers
void drawText(const GFXfont* font, const char* text, int32_t x, int32_t y, uint8_t color = COLOR_BLACK, uint8_t bg = COLOR_WHITE);
void drawTextRight(const GFXfont* font, const char* text, int32_t rightX, int32_t y, uint8_t color = COLOR_BLACK, uint8_t bg = COLOR_WHITE);
void drawTextCentered(const GFXfont* font, const char* text, int32_t centerX, int32_t y, uint8_t color = COLOR_BLACK, uint8_t bg = COLOR_WHITE);
int32_t getTextWidth(const GFXfont* font, const char* text);

// Shape helpers
void drawHLine(int32_t x, int32_t y, int32_t w, uint8_t color = COLOR_BLACK);
void drawButton(int32_t x, int32_t y, int32_t w, int32_t h, const char* label, bool filled = false);
void drawCheckbox(int32_t x, int32_t y, bool checked);

// Icon helpers (48x48 weather icons)
void drawWeatherIcon(int32_t x, int32_t y, int weatherCode, bool invert = false);

// Status bar
void drawStatusBar();
