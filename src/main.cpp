#include <Arduino.h>
#include "epd_driver.h"
#include "fonts.h"
#include "app_state.h"
#include "touch_handler.h"
#include "display_common.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "weather_service.h"
#include "time_utils.h"
#include "screen_clock.h"
#include "screen_forecast.h"
#include "screen_config.h"
#include "screen_keyboard.h"
#include "serial_cmd.h"

// Update intervals
static const unsigned long WEATHER_INTERVAL_MS = 10 * 60 * 1000;  // 10 minutes
static const unsigned long NTP_RESYNC_MS       = 60 * 60 * 1000;  // 1 hour
static const unsigned long ANTI_GHOST_UPDATES  = 10;               // Full clear every N partial updates

// Track which minute we last rendered so we update exactly on minute change
static int8_t lastRenderedMinute = -1;

// Background task handle
static TaskHandle_t networkTaskHandle = NULL;
static volatile bool networkBusy = false;

// Network task: runs weather fetches in background
static void networkTask(void* param) {
    for (;;) {
        if (!networkBusy && g_state.wifiConnected) {
            unsigned long now = millis();
            // Weather fetch
            if (now - g_state.lastWeatherFetch > WEATHER_INTERVAL_MS || g_state.lastWeatherFetch == 0) {
                networkBusy = true;
                fetchAllWeather();
                networkBusy = false;
                g_state.needsFullRedraw = true;
            }
            // NTP resync
            if (now - g_state.lastNtpSync > NTP_RESYNC_MS) {
                networkBusy = true;
                ntpSync();
                networkBusy = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
}

// Update local times for all configured cities
static void updateLocalTimes() {
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        const CityPreset& preset = PRESET_CITIES[g_state.cities[i].presetIndex];
        getLocalTimeForTZ(preset.posixTz, &g_state.cities[i].localTime);
    }
}

// Render current screen
static void renderScreen() {
    switch (g_state.currentScreen) {
        case SCREEN_CLOCK:    screenClockRender();    break;
        case SCREEN_FORECAST: screenForecastRender(); break;
        case SCREEN_CONFIG:   screenConfigRender();   break;
        case SCREEN_KEYBOARD: screenKeyboardRender(); break;
    }
    g_state.needsFullRedraw = false;
    g_state.needsPartialUpdate = false;
}

// Handle touch for current screen
static void handleTouch(int16_t x, int16_t y) {
    switch (g_state.currentScreen) {
        case SCREEN_CLOCK:    screenClockHandleTouch(x, y);    break;
        case SCREEN_FORECAST: screenForecastHandleTouch(x, y); break;
        case SCREEN_CONFIG:   screenConfigHandleTouch(x, y);   break;
        case SCREEN_KEYBOARD: screenKeyboardHandleTouch(x, y); break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ClockyClock Starting ===");

    // Initialize display
    epd_init();

    // Initialize touch early (before any epd_poweroff calls)
    bool touchOk = touchInit();
    Serial.printf("Touch: %s\n", touchOk ? "OK" : "FAILED");

    // Allocate framebuffer in PSRAM
    g_state.framebuffer = (uint8_t*)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!g_state.framebuffer) {
        Serial.println("FATAL: Framebuffer allocation failed!");
        while (1) delay(1000);
    }
    Serial.println("Framebuffer allocated (PSRAM)");

    // Initialize cities with built-in defaults, then state
    citiesLoadDefaults();
    initAppState();

    // Show boot screen
    clearFramebuffer();
    drawTextCentered(FONT_LG, "ClockyClock", SCREEN_W / 2, SCREEN_H / 2 - 20, COLOR_BLACK);
    drawTextCentered(FONT_MD, "Initializing...", SCREEN_W / 2, SCREEN_H / 2 + 40, COLOR_MID);
    pushFramebuffer();

    // Initialize SD card and load cities + config
    sdCardInit();
    if (g_state.sdCardPresent) {
        citiesLoad();
        configLoad();
        configApply();
    }

    // Connect WiFi
    if (strlen(g_state.config.wifiSsid) > 0) {
        // Show connecting status
        clearFramebuffer();
        drawTextCentered(FONT_LG, "ClockyClock", SCREEN_W / 2, SCREEN_H / 2 - 60, COLOR_BLACK);
        char wifiBuf[80];
        snprintf(wifiBuf, sizeof(wifiBuf), "Connecting to %s...", g_state.config.wifiSsid);
        drawTextCentered(FONT_MD, wifiBuf, SCREEN_W / 2, SCREEN_H / 2 + 10, COLOR_MID);
        pushFramebuffer();

        wifiConnect(g_state.config.wifiSsid, g_state.config.wifiPassword);

        if (g_state.wifiConnected) {
            ntpSync();
        }
    }

    // Start background network task
    xTaskCreatePinnedToCore(
        networkTask,
        "network",
        8192,
        NULL,
        1,
        &networkTaskHandle,
        0  // Run on core 0 (app runs on core 1)
    );

    // Initial time update
    updateLocalTimes();

    // Render initial screen
    g_state.needsFullRedraw = true;
    g_state.lastDisplayUpdate = millis();

    serialCmdInit();
    Serial.println("=== ClockyClock Ready ===");
}

void loop() {
    unsigned long now = millis();

    // Poll serial commands
    serialCmdPoll();

    // Poll touch
    int16_t tx, ty;
    if (touchPoll(tx, ty)) {
        Serial.printf("Touch: (%d, %d)\n", tx, ty);
        handleTouch(tx, ty);
    }

    // Time update: sync to real clock — update 1 second after each minute change
    if (g_state.currentScreen == SCREEN_CLOCK && g_state.ntpSynced) {
        struct tm utcNow;
        time_t t = time(NULL);
        gmtime_r(&t, &utcNow);
        // Trigger when second == 1 and we haven't rendered this minute yet
        if (utcNow.tm_sec >= 1 && utcNow.tm_min != lastRenderedMinute) {
            lastRenderedMinute = utcNow.tm_min;
            updateLocalTimes();
            g_state.partialUpdateCount++;

            if (g_state.partialUpdateCount >= ANTI_GHOST_UPDATES) {
                g_state.needsFullRedraw = true;
                g_state.partialUpdateCount = 0;
            } else {
                g_state.needsPartialUpdate = true;
            }
        }
    }

    // Render if needed
    if (g_state.needsFullRedraw || g_state.needsPartialUpdate) {
        updateLocalTimes();
        renderScreen();
    }

    delay(20);  // ~50Hz poll rate
}
