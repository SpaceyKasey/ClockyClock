#include "weather_service.h"
#include "cities.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static const char* OPEN_METEO_BASE = "https://api.open-meteo.com/v1/forecast";
static const unsigned long WEATHER_RETRY_MS = 9 * 60 * 1000;  // On failure, retry in ~60s (10min interval - 9min)

bool fetchWeather(CityData& city, bool useFahrenheit) {
    if (WiFi.status() != WL_CONNECTED) return false;

    const CityPreset& preset = PRESET_CITIES[city.presetIndex];
    const char* tempUnit = useFahrenheit ? "fahrenheit" : "celsius";

    char url[512];
    snprintf(url, sizeof(url),
             "%s?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m,relative_humidity_2m"
             "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_hours,precipitation_probability_max"
             "&forecast_days=7"
             "&timezone=auto"
             "&temperature_unit=%s"
             "&wind_speed_unit=%s",
             OPEN_METEO_BASE, preset.latitude, preset.longitude, tempUnit,
             useFahrenheit ? "mph" : "kmh");

    Serial.printf("Weather: Fetching for %s...\n", preset.name);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);
    int httpCode = http.GET();

    if (httpCode <= 0) {
        // Network error — no response at all
        Serial.printf("Weather: Network error %d for %s\n", httpCode, preset.name);
        http.end();
        return false;
    }
    if (httpCode != 200) {
        // Got a response but not OK — API reachable, don't need to retry
        Serial.printf("Weather: HTTP %d for %s\n", httpCode, preset.name);
        http.end();
        return true;  // count as "attempted" so we don't hammer
    }

    String payload = http.getString();
    http.end();

    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("Weather: JSON parse error: %s\n", err.c_str());
        return false;
    }

    // Current conditions
    JsonObject current = doc["current"];
    city.currentTemp = current["temperature_2m"] | 0.0f;
    city.currentWeatherCode = current["weather_code"] | -1;
    city.currentWindSpeed = current["wind_speed_10m"] | 0.0f;
    city.currentWindDir = current["wind_direction_10m"] | 0;
    city.currentHumidity = current["relative_humidity_2m"] | 0;

    // Daily forecast
    JsonObject daily = doc["daily"];
    JsonArray codes = daily["weather_code"];
    JsonArray maxTemps = daily["temperature_2m_max"];
    JsonArray minTemps = daily["temperature_2m_min"];
    JsonArray sunrises = daily["sunrise"];
    JsonArray sunsets = daily["sunset"];
    JsonArray precipHours = daily["precipitation_hours"];
    JsonArray precipProb = daily["precipitation_probability_max"];

    for (int i = 0; i < 7 && i < (int)codes.size(); i++) {
        city.forecast[i].code = codes[i] | -1;
        city.forecast[i].tempMax = maxTemps[i] | 0.0f;
        city.forecast[i].tempMin = minTemps[i] | 0.0f;
        city.forecast[i].precipHours = precipHours[i] | 0.0f;
        city.forecast[i].precipChance = precipProb[i] | 0;
    }

    // Store today's sunrise/sunset (format: "2024-04-06T06:23")
    const char* sr = sunrises[0] | "";
    const char* ss = sunsets[0] | "";
    // Extract time portion after 'T'
    const char* srT = strchr(sr, 'T');
    const char* ssT = strchr(ss, 'T');
    if (srT) strncpy(city.sunrise, srT + 1, 5);
    if (ssT) strncpy(city.sunset, ssT + 1, 5);
    city.sunrise[5] = '\0';
    city.sunset[5] = '\0';

    city.weatherValid = true;
    city.weatherLastUpdate = millis();

    Serial.printf("Weather: %s - %.1f%s, code %d\n",
                  preset.name, city.currentTemp,
                  useFahrenheit ? "F" : "C", city.currentWeatherCode);

    return true;
}

void fetchAllWeather() {
    bool networkFailed = false;
    bool anyUpdated = false;
    for (uint8_t i = 0; i < g_state.config.numCities; i++) {
        if (!fetchWeather(g_state.cities[i], g_state.config.useFahrenheit)) {
            networkFailed = true;
            break;  // network down, stop trying remaining cities
        }
        anyUpdated = true;
        delay(200);
    }
    if (!networkFailed) {
        g_state.lastWeatherFetch = millis();
    } else {
        // Back off on failure: retry in 60s instead of hammering every 5s
        g_state.lastWeatherFetch = millis() - WEATHER_RETRY_MS;
    }
    // Only redraw if we actually got new data
    if (anyUpdated) {
        g_state.needsFullRedraw = true;
    }
}
