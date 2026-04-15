#include "wifi_manager.h"
#include "app_state.h"
#include "time_utils.h"
#include <WiFi.h>
#include <esp_sntp.h>

bool wifiConnect(const char* ssid, const char* password, int timeoutSec) {
    if (strlen(ssid) == 0) {
        Serial.println("WiFi: No SSID configured");
        return false;
    }

    Serial.printf("WiFi: Connecting to '%s'...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int elapsed = 0;
    while (WiFi.status() != WL_CONNECTED && elapsed < timeoutSec * 2) {
        delay(500);
        elapsed++;
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi: Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        g_state.wifiConnected = true;
        return true;
    }

    Serial.println("WiFi: Connection failed");
    g_state.wifiConnected = false;
    return false;
}

void wifiDisconnect() {
    WiFi.disconnect(true);
    g_state.wifiConnected = false;
}

bool wifiIsConnected() {
    bool connected = (WiFi.status() == WL_CONNECTED);
    g_state.wifiConnected = connected;
    return connected;
}

bool ntpSync() {
    if (!wifiIsConnected()) return false;

    Serial.println("NTP: Syncing...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Wait for time to be set
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        delay(500);
        retry++;
    }

    if (timeinfo.tm_year > 100) {
        Serial.printf("NTP: Synced - UTC %04d-%02d-%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        g_state.ntpSynced = true;
        g_state.lastNtpSync = millis();
        rtcSyncFromSystem();
        return true;
    }

    Serial.println("NTP: Sync failed");
    return false;
}
