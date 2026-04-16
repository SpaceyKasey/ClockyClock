#pragma once
#include <stdint.h>

// Neko behavior states
enum NekoState : uint8_t {
    NEKO_AWAKE = 0,
    NEKO_YAWNING = 1,
    NEKO_SLEEPING = 2,
    NEKO_WAKING = 3,
};

void nekoUpdate();
void nekoDrawAt(int32_t x, int32_t y);
