#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/game.c"

int screen_w = 1280;
int screen_h = 720;
double dt = 1.0 / 60.0;
int mouse_clicked;
double ds_mouse_x;
double ds_mouse_y;

static int tri_calls;
static int snd_play_calls;

double clamp(double value, double low, double high) {
    return value < low ? low : value > high ? high : value;
}

void ds_log(const char *format, ...) {
    (void)format;
}

void ds_runtime_error(const char *format, ...) {
    (void)format;
    abort();
}

void ds_set_asset_manager(AAssetManager *assets) {
    (void)assets;
}

void rect(float x, float y, float w, float h, uint32_t color) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}

void circle(float x, float y, float r, uint32_t color) {
    (void)x;
    (void)y;
    (void)r;
    (void)color;
}

void ring(float x, float y, float r, float t, uint32_t color) {
    (void)x;
    (void)y;
    (void)r;
    (void)t;
    (void)color;
}

void text(const char *string, float x, float y, uint32_t color) {
    (void)string;
    (void)x;
    (void)y;
    (void)color;
}

void text_scaled(const char *string, float x, float y, uint32_t color, float scale) {
    (void)string;
    (void)x;
    (void)y;
    (void)color;
    (void)scale;
}

int text_ink_width(const char *string) {
    return (int)strlen(string) * 10;
}

void clear_screen(uint32_t color) {
    (void)color;
}

void tri(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) {
    (void)color;
    assert(isfinite(x1) && isfinite(y1) && isfinite(x2) && isfinite(y2) && isfinite(x3) && isfinite(y3));
    tri_calls++;
}

int snd_load(const char *name) {
    return name && *name;
}

int snd_play(const char *name) {
    (void)name;
    snd_play_calls++;
    return 1;
}

int main(void) {
    double x0, z0, step, yaw0;
    double sx, sy, f, dist;
    int i, base;
    M4 m, v;
    V3 cam, t;

    game_init(NULL);
    assert(w3_sx == 130 && w3_sy == screen_h - 150 && w3_sr == 70);
    assert(w3_jx == screen_w - 140 && w3_jy == screen_h - 150);
    assert(w3_cam_yaw == M3_PI && w3_snd == 1);

    /* The look-at target projects to the screen center. */
    w3_cam_view(&v, &cam);
    t = m3_pt(&v, m3_v(w3_px, 1.6, w3_pz));
    assert(fabs(t.x) < 1e-9 && fabs(t.y) < 1e-9 && t.z < 0);

    /* Stick right walks at full unit speed. */
    touch_world(w3_sx + 70, w3_sy, 0, 3);
    assert(w3_sdx == 1 && w3_sdy == 0);
    x0 = w3_px;
    z0 = w3_pz;
    game_update();
    assert(w3_moving == 1);
    step = sqrt((w3_px - x0) * (w3_px - x0) + (w3_pz - z0) * (w3_pz - z0));
    assert(fabs(step - W3_SPEED * dt) < 1e-9);
    /* Near-center push travels exactly as far as the rim push. */
    touch_world(w3_sx + 14, w3_sy, 2, 3);
    assert(w3_sdx == 1 && w3_sdy == 0);
    x0 = w3_px;
    z0 = w3_pz;
    world_update();
    dist = sqrt((w3_px - x0) * (w3_px - x0) + (w3_pz - z0) * (w3_pz - z0));
    assert(fabs(dist - step) < 1e-9);
    /* Deadzone holds still, release resets the stick. */
    touch_world(w3_sx + 5, w3_sy, 2, 3);
    assert(w3_sdx == 0 && w3_sdy == 0);
    x0 = w3_px;
    world_update();
    assert(w3_px == x0 && w3_moving == 0);
    touch_world(0, 0, 1, 3);
    assert(w3_sid == -1 && w3_sdx == 0);

    /* Tap on the green button jumps and lands back on the plate. */
    touch_world(w3_jx, w3_jy, 0, 5);
    assert(w3_jid == 5);
    touch_world(w3_jx, w3_jy, 1, 5);
    assert(w3_vy > 0 && w3_py > 0 && w3_jid == -1 && snd_play_calls == 1);
    i = 0;
    while (i < 200) {
        world_update();
        i = i + 1;
    }
    assert(w3_py == 0 && w3_vy == 0);

    /* A drag started on the button becomes a camera orbit, not a jump. */
    touch_world(w3_jx, w3_jy, 0, 6);
    touch_world(w3_jx + 40, w3_jy, 2, 6);
    assert(w3_jid == -1 && w3_oid == 6);
    yaw0 = w3_cam_yaw;
    touch_world(w3_jx + 90, w3_jy, 2, 6);
    assert(fabs((w3_cam_yaw - yaw0) - (0 - 50 * 0.008)) < 1e-9);
    touch_world(0, 0, 1, 6);
    assert(w3_oid == -1 && w3_vy == 0);

    /* Free drag orbits with clamped pitch. */
    w3_cam_yaw = M3_PI;
    w3_cam_pitch = 0.42;
    touch_world(640, 100, 0, 7);
    assert(w3_oid == 7);
    touch_world(640, 10000, 2, 7);
    assert(w3_cam_pitch == 1.15);
    touch_world(640, -10000, 2, 7);
    assert(w3_cam_pitch == 0.12);
    touch_world(0, 0, 1, 7);
    assert(w3_oid == -1);

    /* Blocks are solid: walking into one pushes the player out. */
    w3_cam_yaw = M3_PI;
    w3_px = w3_bx[0] + w3_bhx[0] + 0.4;
    w3_pz = w3_bz[0];
    touch_world(w3_sx + 70, w3_sy, 0, 9);
    i = 0;
    while (i < 30) {
        world_update();
        i = i + 1;
    }
    touch_world(0, 0, 1, 9);
    assert(w3_px >= w3_bx[0] + w3_bhx[0] + 0.44);

    /* A full frame draws the plate, boxes, avatar and controls. */
    tri_calls = 0;
    game_draw(0);
    assert(tri_calls > 100);

    /* Projection: view center maps to the screen center. */
    r3_begin();
    r3_proj(0, 0, -10, &sx, &sy);
    assert(fabs(sx - 640) < 1e-6 && fabs(sy - 360) < 1e-6);
    f = 360.0 / tan(R3_FOV * 0.5);
    r3_proj(2, 1, -10, &sx, &sy);
    assert(fabs(sx - (640 + 2 * f / 10)) < 1e-6);
    assert(fabs(sy - (360 - 1 * f / 10)) < 1e-6);

    /* Fully-behind triangles emit nothing, crossing ones get clipped. */
    base = tri_calls;
    r3_tri(0, 0, 5, 1, 0, 5, 0, 1, 5, 0xFF0000);
    assert(tri_calls == base);
    r3_tri(0, 0, -5, 1, 0, -5, 0, 0, 5, 0xFF0000);
    assert(tri_calls > base);

    /* A box behind the camera emits nothing and leaves the pool clean. */
    m3_translate(&m, r3_cam.x, r3_cam.y, r3_cam.z - 50.0);
    r3_collect(&m, 1, 1, 1, 0xFF0000);
    base = tri_calls;
    r3_flush();
    assert(tri_calls == base && r3_n == 0);

    assert(game_back() == 0);
    game_reset();
    assert(w3_px == 0 && w3_pz == 0 && w3_cam_yaw == M3_PI);

    puts("Injoer 3D world: normal");
    return 0;
}
