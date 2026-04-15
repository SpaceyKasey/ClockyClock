#include "screen_keyboard.h"
#include "display_common.h"
#include "touch_handler.h"
#include <string.h>
#include <stdio.h>

// Keyboard layout
static const char* ROWS_LOWER[] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm",
};
static const char* ROWS_UPPER[] = {
    "1234567890",
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM",
};
static const char* ROWS_SYMBOLS[] = {
    "!@#$%^&*()",
    "-_=+[]{}\\|",
    ";:'\",.<>/?",
    "~`",
};

// Key dimensions
static const int KEY_W = 80;
static const int KEY_H = 55;
static const int KEY_GAP = 8;
static const int KB_Y_START = 170;  // Y offset for first keyboard row
static const int KB_X_START = 20;

// Touch zones - plenty of keys
static TouchZone kbZones[64];
static uint8_t kbZoneCount = 0;

static const char** getRows() {
    if (g_state.symbolsActive) return ROWS_SYMBOLS;
    if (g_state.shiftActive) return ROWS_UPPER;
    return ROWS_LOWER;
}

static void drawKey(int32_t x, int32_t y, int32_t w, const char* label, bool highlight = false) {
    if (highlight) {
        epd_fill_rect(x, y, w, KEY_H, COLOR_DARK, g_state.framebuffer);
        drawTextCentered(FONT_SM, label, x + w / 2, y + KEY_H / 2 + 10, COLOR_WHITE);
    } else {
        epd_draw_rect(x, y, w, KEY_H, COLOR_BLACK, g_state.framebuffer);
        drawTextCentered(FONT_SM, label, x + w / 2, y + KEY_H / 2 + 10, COLOR_BLACK);
    }
}

void screenKeyboardRender() {
    clearFramebuffer();
    kbZoneCount = 0;

    // Header
    drawButton(10, 5, 90, 40, "Cancel");
    kbZones[kbZoneCount++] = {10, 5, 90, 40, ZONE_CANCEL_KEY, 0};

    const char* fieldLabel = g_state.inputField == 0 ? "Enter WiFi SSID:" : "Enter WiFi Password:";
    drawText(FONT_MD, fieldLabel, 120, 38, COLOR_BLACK);

    drawButton(SCREEN_W - 100, 5, 90, 40, "Done", true);
    kbZones[kbZoneCount++] = {(int16_t)(SCREEN_W - 100), 5, 90, 40, ZONE_DONE_KEY, 0};

    // Text input field
    int32_t fieldY = 60;
    epd_draw_rect(20, fieldY, SCREEN_W - 40, 50, COLOR_BLACK, g_state.framebuffer);
    epd_fill_rect(22, fieldY + 2, SCREEN_W - 44, 46, COLOR_WHITE, g_state.framebuffer);

    // Show input text (mask password with *)
    char displayBuf[64];
    if (g_state.inputField == 1) {
        // Password - show last char, mask rest
        size_t len = strlen(g_state.inputBuffer);
        for (size_t i = 0; i < len && i < sizeof(displayBuf) - 1; i++) {
            displayBuf[i] = (i == len - 1) ? g_state.inputBuffer[i] : '*';
        }
        displayBuf[len] = '\0';
    } else {
        strncpy(displayBuf, g_state.inputBuffer, sizeof(displayBuf));
    }

    // Add cursor
    char withCursor[70];
    snprintf(withCursor, sizeof(withCursor), "%s|", displayBuf);
    drawText(FONT_MD, withCursor, 30, fieldY + 38, COLOR_BLACK);

    // Character count
    char countBuf[8];
    snprintf(countBuf, sizeof(countBuf), "%d/63", (int)strlen(g_state.inputBuffer));
    drawTextRight(FONT_SM, countBuf, SCREEN_W - 30, fieldY + 38, COLOR_MID);

    // Keyboard rows
    const char** rows = getRows();

    for (int row = 0; row < 4; row++) {
        const char* keys = rows[row];
        int numKeys = strlen(keys);
        int32_t y = KB_Y_START + row * (KEY_H + KEY_GAP);

        // Center the row
        int totalW = numKeys * KEY_W + (numKeys - 1) * KEY_GAP;
        int32_t startX = (SCREEN_W - totalW) / 2;

        // Special keys for row 3 (shift + backspace)
        if (row == 3) {
            // Shift key on left
            int shiftW = KEY_W + 20;
            drawKey(KB_X_START, y, shiftW, g_state.shiftActive ? "SHIFT" : "Shift", g_state.shiftActive);
            kbZones[kbZoneCount++] = {(int16_t)KB_X_START, (int16_t)y, (int16_t)shiftW, KEY_H, ZONE_SHIFT_KEY, 0};
            startX = KB_X_START + shiftW + KEY_GAP;

            // Regular keys
            for (int k = 0; k < numKeys; k++) {
                int32_t kx = startX + k * (KEY_W + KEY_GAP);
                char label[2] = {keys[k], '\0'};
                drawKey(kx, y, KEY_W, label);
                kbZones[kbZoneCount++] = {(int16_t)kx, (int16_t)y, KEY_W, KEY_H, ZONE_KEY, (uint8_t)keys[k]};
            }

            // Backspace key on right
            int bkspX = startX + numKeys * (KEY_W + KEY_GAP);
            int bkspW = SCREEN_W - KB_X_START - bkspX;
            if (bkspW < KEY_W) bkspW = KEY_W + 20;
            drawKey(bkspX, y, bkspW, "DEL");
            kbZones[kbZoneCount++] = {(int16_t)bkspX, (int16_t)y, (int16_t)bkspW, KEY_H, ZONE_BACKSPACE_KEY, 0};
        } else {
            // Normal row
            for (int k = 0; k < numKeys; k++) {
                int32_t kx = startX + k * (KEY_W + KEY_GAP);
                char label[2] = {keys[k], '\0'};
                drawKey(kx, y, KEY_W, label);
                kbZones[kbZoneCount++] = {(int16_t)kx, (int16_t)y, KEY_W, KEY_H, ZONE_KEY, (uint8_t)keys[k]};
            }
        }
    }

    // Bottom row: symbols toggle, space bar, common chars
    int32_t bottomY = KB_Y_START + 4 * (KEY_H + KEY_GAP);

    // Symbols toggle
    drawKey(KB_X_START, bottomY, KEY_W + 20, g_state.symbolsActive ? "abc" : "!@#", g_state.symbolsActive);
    kbZones[kbZoneCount++] = {(int16_t)KB_X_START, (int16_t)bottomY, (int16_t)(KEY_W + 20), KEY_H, ZONE_SYMBOLS_KEY, 0};

    // Space bar
    int spaceX = KB_X_START + KEY_W + 20 + KEY_GAP;
    int spaceW = 400;
    drawKey(spaceX, bottomY, spaceW, "SPACE");
    kbZones[kbZoneCount++] = {(int16_t)spaceX, (int16_t)bottomY, (int16_t)spaceW, KEY_H, ZONE_SPACE_KEY, 0};

    // Common chars: . @ - _
    const char* extras = ".@-_";
    int extraX = spaceX + spaceW + KEY_GAP;
    for (int i = 0; i < 4; i++) {
        int32_t ex = extraX + i * (KEY_W + KEY_GAP);
        char label[2] = {extras[i], '\0'};
        drawKey(ex, bottomY, KEY_W, label);
        kbZones[kbZoneCount++] = {(int16_t)ex, (int16_t)bottomY, KEY_W, KEY_H, ZONE_KEY, (uint8_t)extras[i]};
    }

    pushFramebuffer();
}

void screenKeyboardHandleTouch(int16_t x, int16_t y) {
    const TouchZone* zone = hitTest(x, y, kbZones, kbZoneCount);
    if (!zone) return;

    switch (zone->id) {
        case ZONE_KEY: {
            size_t len = strlen(g_state.inputBuffer);
            if (len < sizeof(g_state.inputBuffer) - 1) {
                g_state.inputBuffer[len] = (char)zone->data;
                g_state.inputBuffer[len + 1] = '\0';
                g_state.inputCursor = len + 1;
                // Auto-disable shift after one character
                if (g_state.shiftActive) g_state.shiftActive = false;
            }
            g_state.needsFullRedraw = true;
            break;
        }

        case ZONE_SPACE_KEY: {
            size_t len = strlen(g_state.inputBuffer);
            if (len < sizeof(g_state.inputBuffer) - 1) {
                g_state.inputBuffer[len] = ' ';
                g_state.inputBuffer[len + 1] = '\0';
                g_state.inputCursor = len + 1;
            }
            g_state.needsFullRedraw = true;
            break;
        }

        case ZONE_BACKSPACE_KEY: {
            size_t len = strlen(g_state.inputBuffer);
            if (len > 0) {
                g_state.inputBuffer[len - 1] = '\0';
                g_state.inputCursor = len - 1;
            }
            g_state.needsFullRedraw = true;
            break;
        }

        case ZONE_SHIFT_KEY:
            g_state.shiftActive = !g_state.shiftActive;
            if (g_state.symbolsActive) g_state.symbolsActive = false;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_SYMBOLS_KEY:
            g_state.symbolsActive = !g_state.symbolsActive;
            if (g_state.shiftActive) g_state.shiftActive = false;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_DONE_KEY:
            // Save input to the appropriate field
            if (g_state.inputField == 0) {
                strncpy(g_state.config.wifiSsid, g_state.inputBuffer, sizeof(g_state.config.wifiSsid));
            } else {
                strncpy(g_state.config.wifiPassword, g_state.inputBuffer, sizeof(g_state.config.wifiPassword));
            }
            g_state.currentScreen = SCREEN_CONFIG;
            g_state.needsFullRedraw = true;
            break;

        case ZONE_CANCEL_KEY:
            g_state.currentScreen = SCREEN_CONFIG;
            g_state.needsFullRedraw = true;
            break;
    }
}
