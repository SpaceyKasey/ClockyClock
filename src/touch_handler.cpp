#include "touch_handler.h"
#include "pins.h"
#include <Wire.h>
#include <TouchDrvGT911.hpp>
#include <esp_sleep.h>

static TouchDrvGT911 touch;
static bool touchReady = false;
static unsigned long lastTouchTime = 0;
static const unsigned long DEBOUNCE_MS = 250;

bool touchInit() {
    // If waking from sleep, wait for touch controller to come up
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
        delay(1000);
    }

    Wire.begin(BOARD_SDA, BOARD_SCL);

    // Drive INT HIGH to wake GT911 from sleep
    pinMode(TOUCH_INT, OUTPUT);
    digitalWrite(TOUCH_INT, HIGH);
    delay(50);

    // Scan both addresses
    uint8_t addr = 0;
    Wire.beginTransmission(0x14);
    if (Wire.endTransmission() == 0) addr = 0x14;
    Wire.beginTransmission(0x5D);
    if (Wire.endTransmission() == 0) addr = 0x5D;

    if (addr == 0) {
        Serial.println("Touch: GT911 not found");
        return false;
    }

    Serial.printf("Touch: GT911 found at 0x%02X\n", addr);

    touch.setPins(-1, TOUCH_INT);
    if (!touch.begin(Wire, addr, BOARD_SDA, BOARD_SCL)) {
        Serial.println("Touch: begin() failed");
        return false;
    }

    touch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, true);

    Serial.println("Touch: initialized OK");
    touchReady = true;
    return true;
}

bool touchPoll(int16_t& x, int16_t& y) {
    if (!touchReady) return false;

    unsigned long now = millis();
    if (now - lastTouchTime < DEBOUNCE_MS) {
        return false;
    }

    int16_t tx, ty;
    uint8_t touched = touch.getPoint(&tx, &ty, 1);
    if (touched) {
        lastTouchTime = now;
        x = tx;
        y = ty;
        return true;
    }
    return false;
}

const TouchZone* hitTest(int16_t x, int16_t y, const TouchZone* zones, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        const TouchZone& z = zones[i];
        if (x >= z.x && x < z.x + z.w && y >= z.y && y < z.y + z.h) {
            return &z;
        }
    }
    return nullptr;
}
