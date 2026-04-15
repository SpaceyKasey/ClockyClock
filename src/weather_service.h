#pragma once
#include "app_state.h"

// Fetch weather for a single city, populating its CityData
bool fetchWeather(CityData& city, bool useFahrenheit);

// Fetch weather for all configured cities
void fetchAllWeather();
