#pragma once
#include "app_state.h"

bool touchInit();
bool touchPoll(int16_t& x, int16_t& y);

// Hit-test a point against an array of zones
// Returns the zone that was hit, or nullptr
const TouchZone* hitTest(int16_t x, int16_t y, const TouchZone* zones, uint8_t count);
