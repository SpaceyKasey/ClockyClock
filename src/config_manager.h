#pragma once
#include "app_state.h"

bool sdCardInit();
bool configSave();
bool configLoad();

// Load cities.json from SD card; writes defaults if file missing
bool citiesLoad();

// Reset cities.json on SD to firmware defaults
void citiesReset();

// Save current cities list to SD
void citiesSave();

// Called after config changes to rebuild city data array
void configApply();
