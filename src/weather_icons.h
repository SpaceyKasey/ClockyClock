#pragma once
#include <stdint.h>

#define ICON_SIZE 48

// Returns pointer to a 48x48 4-bit grayscale icon bitmap for the given WMO weather code
// Returns nullptr for unknown codes
const uint8_t* getWeatherIcon(int wmoCode);

// Get human-readable weather description from WMO code
const char* getWeatherDescription(int wmoCode);
