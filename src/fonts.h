#pragma once
#include "epd_driver.h"

// Font pointers - initialized in fonts.cpp
// These provide external access to font data that has internal linkage
extern const GFXfont* FONT_LG;   // 40pt bold - time display
extern const GFXfont* FONT_MD;   // ~20pt - city names, dates
extern const GFXfont* FONT_SM;   // 14pt - labels, keyboard, status
