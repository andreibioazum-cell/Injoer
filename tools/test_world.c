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
Joy joy;

static int draw_calls;

struct DSArray {
    double *data;
    size_t length;
    size_t capacity;
};

DSArray *arr_new(void) {
    DSArray *array = calloc(1, sizeof(*array));
    assert(array);
    return array;
}

void arr_push(DSArray *array, double value) {
    if (array->length == array->capacity) {
        size_t capacity = array->capacity ? array->capacity * 2 : 8;
        double *data = realloc(array->data, capacity * sizeof(*data));
        assert(data);
        array->data = data;
        array->capacity = capacity;
    }
    array->data[array->length++] = value;
}

double arr_get(DSArray *array, double index) {
    size_t position = (size_t)index;
    return array && position < array->length ? array->data[position] : 0;
}

void arr_set(DSArray *array, double index, double value) {
    size_t position = (size_t)index;
    while (array->length <= position)
        arr_push(array, 0);
    array->data[position] = value;
}

double arr_len(DSArray *array) {
    return array ? (double)array->length : 0;
}

void arr_clear(DSArray *array) {
    if (array)
        array->length = 0;
}

void arr_free(DSArray *array) {
    if (!array)
        return;
    free(array->data);
    free(array);
}

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

void rect(float x, float y, float w, float h, uint32_t color) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
    draw_calls++;
}

void circle(float x, float y, float r, uint32_t color) {
    (void)x;
    (void)y;
    (void)r;
    (void)color;
    draw_calls++;
}

void ring(float x, float y, float r, float t, uint32_t color) {
    (void)x;
    (void)y;
    (void)r;
    (void)t;
    (void)color;
    draw_calls++;
}

void line(float x1, float y1, float x2, float y2, float thickness, uint32_t color) {
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)thickness;
    (void)color;
    draw_calls++;
}

void roundrect(float x, float y, float w, float h, float r, uint32_t color) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)color;
    draw_calls++;
}

void text(const char *string, float x, float y, uint32_t color) {
    (void)string;
    (void)x;
    (void)y;
    (void)color;
    draw_calls++;
}

void text_scaled(const char *string, float x, float y, uint32_t color, float scale) {
    (void)string;
    (void)x;
    (void)y;
    (void)color;
    (void)scale;
    draw_calls++;
}

int text_ink_width(const char *string) {
    return (int)strlen(string) * 10;
}

int text_ink_height(const char *string) {
    (void)string;
    return 20;
}

int text_ink_top(const char *string) {
    (void)string;
    return 0;
}

int snd_play(const char *name) {
    (void)name;
    return 1;
}

int main(void) {
    double x0;
    double step;
    int i;

    state_create();
    state_ready = 1;
    assert(ST_WORLD == 3);
    assert(strcmp(world_mode_label(), "World 3D") == 0);
    language = 1;
    assert(strcmp(world_mode_label(), "Мир 3D") == 0);
    language = 0;

    game_state = ST_MODES;
    init_world();
    assert(joy.x == 130 && joy.y == screen_h - 150 && joy.r == 70);
    assert(w_jump_id == -1 && w_px == 0 && w_pz == 0);

    /* Stick right: the same unit-speed stick as the battle screen. */
    touch_world(joy.x + 70, joy.y, 0, 3);
    assert(joy.dx == 1 && joy.dy == 0);
    x0 = w_px;
    update_world();
    assert(w_px > x0 && w_moving == 1 && w_camx > 0);
    step = w_px - x0;
    assert(fabs(step - (joy_speed / 50) * dt) < 1e-9);
    /* Near-center push travels exactly as far as the rim push. */
    touch_world(joy.x + 14, joy.y, 2, 3);
    assert(joy.dx == 1 && joy.dy == 0);
    x0 = w_px;
    update_world();
    assert(fabs((w_px - x0) - step) < 1e-9);
    /* Release stops the walk. */
    touch_world(0, 0, 1, 3);
    assert(joy.dx == 0 && joy.dy == 0);
    x0 = w_px;
    update_world();
    assert(w_px == x0 && w_moving == 0);

    /* Jump button lobs the avatar up and lands back on the plate. */
    touch_world(w_jump_x, w_jump_y, 0, 5);
    assert(w_vy > 0 && w_py > 0);
    touch_world(0, 0, 1, 5);
    assert(w_jump_id == -1);
    i = 0;
    while (i < 600) {
        update_world();
        i = i + 1;
    }
    assert(w_py == 0 && w_vy == 0);

    /* Blocks are solid: walking into one pushes the player out. */
    w_px = wb_x[0] + wb_half[0] + 0.4;
    w_pz = wb_z[0];
    touch_world(joy.x - 70, joy.y, 0, 9);
    i = 0;
    while (i < 30) {
        update_world();
        i = i + 1;
    }
    touch_world(0, 0, 1, 9);
    assert(w_px >= wb_x[0] + wb_half[0] + 0.44);

    /* A full frame draws the plate, blocks, avatar and controls. */
    draw_calls = 0;
    draw_world();
    assert(draw_calls > 50);

    /* The back button returns to the mode list. */
    t_dir = 0;
    touch_world(screen_w / 2, back_y + 10, 0, 7);
    assert(t_dir == 1 && t_target == ST_MODES);

    state_destroy();
    state_ready = 0;
    puts("Injoer 3D world: normal");
    return 0;
}
