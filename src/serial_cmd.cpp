#include "serial_cmd.h"
#include "app_state.h"
#include "config_manager.h"
#include "cities.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <string.h>

#define CMD_BUF_SIZE 256
#define CRLF "\r\n"

static char cmdBuf[CMD_BUF_SIZE];
static uint8_t cmdPos = 0;

// Print with \r\n line endings for terminal compatibility
static void sout(const char* s) {
    Serial.print(s);
    Serial.print(CRLF);
}

static void printHelp() {
    sout("");
    sout("--- ClockyClock Commands ---");
    sout("  help                       Show this help");
    sout("  status                     Show current config");
    sout("  ssid <name>                Set WiFi SSID");
    sout("  pass <password>            Set WiFi password");
    sout("  connect                    Connect WiFi now");
    sout("  cities                     List available cities");
    sout("  select 0,4,5,7             Select cities by index");
    sout("  addcity <name>|<tz>|<lat>|<lon>");
    sout("                             Add a city to the list");
    sout("  rmcity <index>             Remove a city by index");
    sout("  unit f|c                   Set temperature unit");
    sout("  clock 12|24                Set 12h or 24h time");
    sout("  resetcities                Reset cities to defaults");
    sout("  save                       Save config to SD");
    sout("  savecities                 Save cities to SD");
    sout("  reboot                     Restart device");
    sout("----------------------------");
}

static void printStatus() {
    char buf[128];
    sout("");
    sout("--- Status ---");
    snprintf(buf, sizeof(buf), "  WiFi SSID:  %s", strlen(g_state.config.wifiSsid) > 0 ? g_state.config.wifiSsid : "(not set)");
    sout(buf);
    snprintf(buf, sizeof(buf), "  WiFi Pass:  %s", strlen(g_state.config.wifiPassword) > 0 ? "********" : "(not set)");
    sout(buf);
    snprintf(buf, sizeof(buf), "  WiFi:       %s", g_state.wifiConnected ? "Connected" : "Disconnected");
    sout(buf);
    snprintf(buf, sizeof(buf), "  NTP:        %s", g_state.ntpSynced ? "Synced" : "Not synced");
    sout(buf);
    snprintf(buf, sizeof(buf), "  Unit:       %s", g_state.config.useFahrenheit ? "Fahrenheit" : "Celsius");
    sout(buf);
    snprintf(buf, sizeof(buf), "  Clock:      %s", g_state.config.use24Hour ? "24h" : "12h");
    sout(buf);
    snprintf(buf, sizeof(buf), "  SD Card:    %s", g_state.sdCardPresent ? "Present" : "Not found");
    sout(buf);
    if (g_state.batteryVoltage > 0.5f) {
        snprintf(buf, sizeof(buf), "  Battery:    %.1fV", g_state.batteryVoltage);
        sout(buf);
    }
    snprintf(buf, sizeof(buf), "  Cities (%d):", g_state.config.numCities);
    sout(buf);
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        uint8_t idx = g_state.config.selectedCities[i];
        if (idx < PRESET_COUNT) {
            snprintf(buf, sizeof(buf), "    [%d] %s", idx, PRESET_CITIES[idx].name);
            sout(buf);
        }
    }
    sout("--------------");
}

static void listCities() {
    char buf[128];
    sout("");
    sout("--- Available Cities ---");
    for (uint8_t i = 0; i < PRESET_COUNT; i++) {
        bool sel = false;
        for (uint8_t j = 0; j < g_state.config.numCities; j++) {
            if (g_state.config.selectedCities[j] == i) { sel = true; break; }
        }
        snprintf(buf, sizeof(buf), "  %s[%2d] %-14s %s", sel ? "*" : " ", i,
                 PRESET_CITIES[i].name, PRESET_CITIES[i].posixTz);
        sout(buf);
    }
    snprintf(buf, sizeof(buf), "  (%d/%d slots used)", PRESET_COUNT, MAX_PRESETS);
    sout(buf);
    sout("Use: select 0,4,5,7");
    sout("Add: addcity Name|TZ|lat|lon");
    sout("------------------------");
}

static void handleSelect(const char* arg) {
    char buf[128];
    g_state.config.numCities = 0;
    char tmp[64];
    strlcpy(tmp, arg, sizeof(tmp));
    char* tok = strtok(tmp, ", ");
    while (tok && g_state.config.numCities < 8) {
        int idx = atoi(tok);
        if (idx >= 0 && idx < PRESET_COUNT) {
            g_state.config.selectedCities[g_state.config.numCities++] = (uint8_t)idx;
        }
        tok = strtok(NULL, ", ");
    }
    configApply();
    snprintf(buf, sizeof(buf), "Selected %d cities:", g_state.config.numCities);
    Serial.print(buf);
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        Serial.print(" ");
        Serial.print(PRESET_CITIES[g_state.config.selectedCities[i]].name);
    }
    Serial.print(CRLF);
}

// addcity Name|POSIX_TZ|lat|lon
static void handleAddCity(const char* arg) {
    char buf[128];
    if (PRESET_COUNT >= MAX_PRESETS) {
        snprintf(buf, sizeof(buf), "City list full (%d max)", MAX_PRESETS);
        sout(buf);
        return;
    }

    char tmp[200];
    strlcpy(tmp, arg, sizeof(tmp));

    char* fields[4];
    uint8_t fi = 0;
    fields[0] = tmp;
    for (char* p = tmp; *p && fi < 3; p++) {
        if (*p == '|') {
            *p = '\0';
            fields[++fi] = p + 1;
        }
    }

    if (fi < 3) {
        sout("Usage: addcity Name|TZ|lat|lon");
        sout("  Example: addcity Tokyo|JST-9|35.6762|139.6503");
        return;
    }

    CityPreset& c = PRESET_CITIES[PRESET_COUNT];
    strlcpy(c.name, fields[0], CITY_NAME_LEN);
    strlcpy(c.posixTz, fields[1], CITY_TZ_LEN);
    c.latitude = atof(fields[2]);
    c.longitude = atof(fields[3]);
    PRESET_COUNT++;

    snprintf(buf, sizeof(buf), "Added [%d] %s (%.4f, %.4f) tz=%s",
             PRESET_COUNT - 1, c.name, c.latitude, c.longitude, c.posixTz);
    sout(buf);
    sout("Use 'savecities' to persist, 'select' to display it");
}

static void handleRmCity(const char* arg) {
    char buf[128];
    int idx = atoi(arg);
    if (idx < 0 || idx >= PRESET_COUNT) {
        snprintf(buf, sizeof(buf), "Invalid index %d (0-%d)", idx, PRESET_COUNT - 1);
        sout(buf);
        return;
    }

    char name[CITY_NAME_LEN];
    strlcpy(name, PRESET_CITIES[idx].name, CITY_NAME_LEN);

    for (uint8_t i = idx; i < PRESET_COUNT - 1; i++) {
        PRESET_CITIES[i] = PRESET_CITIES[i + 1];
    }
    PRESET_COUNT--;

    uint8_t newNum = 0;
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        uint8_t si = g_state.config.selectedCities[i];
        if (si == idx) continue;
        if (si > idx) si--;
        g_state.config.selectedCities[newNum++] = si;
    }
    g_state.config.numCities = newNum;
    if (newNum == 0) {
        g_state.config.selectedCities[0] = 0;
        g_state.config.numCities = 1;
    }
    configApply();

    snprintf(buf, sizeof(buf), "Removed '%s'. %d cities remain, %d selected", name, PRESET_COUNT, g_state.config.numCities);
    sout(buf);
    sout("Use 'savecities' and 'save' to persist");
}

static void handleCmd(char* cmd) {
    char buf[128];
    // Trim
    while (*cmd == ' ') cmd++;
    char* end = cmd + strlen(cmd) - 1;
    while (end > cmd && (*end == ' ' || *end == '\r' || *end == '\n')) *end-- = '\0';
    if (strlen(cmd) == 0) return;

    if (strcmp(cmd, "help") == 0) {
        printHelp();
    } else if (strcmp(cmd, "status") == 0) {
        printStatus();
    } else if (strncmp(cmd, "ssid ", 5) == 0) {
        strlcpy(g_state.config.wifiSsid, cmd + 5, sizeof(g_state.config.wifiSsid));
        snprintf(buf, sizeof(buf), "SSID set to: %s", g_state.config.wifiSsid);
        sout(buf);
    } else if (strncmp(cmd, "pass ", 5) == 0) {
        strlcpy(g_state.config.wifiPassword, cmd + 5, sizeof(g_state.config.wifiPassword));
        sout("Password set");
    } else if (strcmp(cmd, "connect") == 0) {
        if (strlen(g_state.config.wifiSsid) == 0) {
            sout("No SSID set");
        } else {
            wifiConnect(g_state.config.wifiSsid, g_state.config.wifiPassword);
            if (g_state.wifiConnected) {
                ntpSync();
                g_state.needsFullRedraw = true;
            }
        }
    } else if (strcmp(cmd, "cities") == 0) {
        listCities();
    } else if (strncmp(cmd, "select ", 7) == 0) {
        handleSelect(cmd + 7);
    } else if (strncmp(cmd, "addcity ", 8) == 0) {
        handleAddCity(cmd + 8);
    } else if (strncmp(cmd, "rmcity ", 7) == 0) {
        handleRmCity(cmd + 7);
    } else if (strncmp(cmd, "clock ", 6) == 0) {
        if (strncmp(cmd + 6, "24", 2) == 0) {
            g_state.config.use24Hour = true;
            sout("Clock: 24h");
            g_state.needsFullRedraw = true;
        } else if (strncmp(cmd + 6, "12", 2) == 0) {
            g_state.config.use24Hour = false;
            sout("Clock: 12h");
            g_state.needsFullRedraw = true;
        } else {
            sout("Usage: clock 12|24");
        }
    } else if (strncmp(cmd, "unit ", 5) == 0) {
        char u = cmd[5];
        if (u == 'f' || u == 'F') {
            g_state.config.useFahrenheit = true;
            sout("Unit: Fahrenheit");
        } else if (u == 'c' || u == 'C') {
            g_state.config.useFahrenheit = false;
            sout("Unit: Celsius");
        } else {
            sout("Usage: unit f|c");
        }
    } else if (strcmp(cmd, "resetcities") == 0) {
        citiesReset();
    } else if (strcmp(cmd, "save") == 0) {
        sout(configSave() ? "Config saved to SD" : "Save failed");
    } else if (strcmp(cmd, "savecities") == 0) {
        citiesSave();
    } else if (strcmp(cmd, "reboot") == 0) {
        sout("Rebooting...");
        delay(100);
        ESP.restart();
    } else {
        snprintf(buf, sizeof(buf), "Unknown: %s (type 'help')", cmd);
        sout(buf);
    }
}

void serialCmdInit() {
    cmdPos = 0;
    sout("Type 'help' for commands");
}

void serialCmdPoll() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (cmdPos > 0) {
                Serial.print(CRLF);  // echo newline
                cmdBuf[cmdPos] = '\0';
                handleCmd(cmdBuf);
                cmdPos = 0;
            }
        } else if (c == '\b' || c == 127) {
            // Backspace
            if (cmdPos > 0) {
                cmdPos--;
                Serial.print("\b \b");  // erase character on screen
            }
        } else if (cmdPos < CMD_BUF_SIZE - 1) {
            cmdBuf[cmdPos++] = c;
            Serial.print(c);  // echo character
        }
    }
}
