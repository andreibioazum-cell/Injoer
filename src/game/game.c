/* Injoer: offline debug 3D sandbox. No menus - boots straight into the world. */
#include "game/game.h"
#include "engine/runtime.h"
#include <math.h>

#include "math3d.inc"
#include "world.inc"
#include "render3d.inc"
#include "input.inc"

void game_init(struct AAssetManager *assets) {
    ds_set_asset_manager(assets);
    world_init();
}

void game_update(void) {
    world_update();
}

void game_draw(struct Buffer *buffer) {
    (void)buffer;
    draw_world();
}

void game_touch(float x, float y, int action, int pointer_id) {
    mouse_clicked = action == 0;
    if (action == 0) {
        ds_mouse_x = x;
        ds_mouse_y = y;
    }
    touch_world((double)x, (double)y, (double)action, (double)pointer_id);
}

int game_back(void) {
    /* Nothing to go back to: let the system close the app. */
    return 0;
}

void game_reset(void) {
    world_init();
}
