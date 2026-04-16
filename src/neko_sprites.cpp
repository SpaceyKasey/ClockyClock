#include "neko_sprites.h"
#include "neko_data.h"
#include "app_state.h"
#include "epd_driver.h"
#include <Arduino.h>

void nekoUpdate() {
    if (g_state.nekoStateTimer > 0) {
        g_state.nekoStateTimer--;
    }

    if (g_state.nekoStateTimer == 0) {
        switch (g_state.nekoState) {
            case NEKO_AWAKE:
                g_state.nekoState = NEKO_YAWNING;
                g_state.nekoStateTimer = 1;
                break;
            case NEKO_YAWNING:
                g_state.nekoState = NEKO_SLEEPING;
                g_state.nekoStateTimer = 5 + (uint8_t)random(8);  // 5-12
                g_state.nekoSleepToggle = false;
                break;
            case NEKO_SLEEPING:
                g_state.nekoState = NEKO_WAKING;
                g_state.nekoStateTimer = 1;
                break;
            case NEKO_WAKING:
                g_state.nekoState = NEKO_AWAKE;
                g_state.nekoStateTimer = 8 + (uint8_t)random(13); // 8-20
                break;
        }
    }

    switch (g_state.nekoState) {
        case NEKO_AWAKE:
            g_state.nekoCurrentSprite = NEKO_AWAKE_POOL[random(NEKO_AWAKE_POOL_COUNT)];
            break;
        case NEKO_YAWNING:
            g_state.nekoCurrentSprite = NEKO_IDX_YAWN2;
            break;
        case NEKO_SLEEPING:
            g_state.nekoCurrentSprite = g_state.nekoSleepToggle ? NEKO_IDX_SLEEP2 : NEKO_IDX_SLEEP1;
            g_state.nekoSleepToggle = !g_state.nekoSleepToggle;
            break;
        case NEKO_WAKING:
            g_state.nekoCurrentSprite = NEKO_IDX_SIT;
            break;
    }
}

void nekoDrawAt(int32_t x, int32_t y) {
    const uint8_t* sprite = NEKO_SPRITES[g_state.nekoCurrentSprite];
    Rect_t area = {x, y, NEKO_SIZE, NEKO_SIZE};
    epd_copy_to_framebuffer(area, (uint8_t*)sprite, g_state.framebuffer);
}
