/* Injoer: offline debug world, a Roblox-like third-person sandbox.
 * Movement uses the exact same virtual joystick as the Cubic Battle fight
 * screen (the shared Joy state plus joy_touch()/drop_joy()), so walking here
 * feels identical to the battle stick: full speed past a small deadzone. */

#define WR_CAM_H 5.0
#define WR_CAM_DIST 11.0
#define WR_HALF 22.0
#define WR_BLOCKS 8

static double w_px;
static double w_pz;
static double w_py;
static double w_vy;
static double w_face;
static double w_walk_t;
static double w_moving;
static double w_camx;
static double w_camz;
static double w_jump_x;
static double w_jump_y;
static double w_jump_id;
static double w_time;

/* Toy blocks scattered over the baseplate: position, half-size of the
 * footprint, height and color. The camera never rotates, so the layout
 * is fixed and doubles as simple cover to walk around. */
static double wb_x[WR_BLOCKS] = {6, -7, 5, -6, 0, 11, -11, 3};
static double wb_z[WR_BLOCKS] = {7, 5, -7, -8, 12, -1, -2, 16};
static double wb_half[WR_BLOCKS] = {1.6, 2.2, 1.4, 2.0, 1.2, 2.4, 1.8, 1.5};
static double wb_h[WR_BLOCKS] = {1.6, 2.6, 3.2, 1.2, 2.0, 1.4, 3.6, 2.2};
static double wb_c[WR_BLOCKS] = {0xE74C3C, 0xF1C40F, 0x3498DB, 0x9B59B6, 0xE67E22, 0x1ABC9C, 0xE91E63,
                                 0x7ED321};

static const char *world_mode_label(void) {
    if (language == 1) {
        return "Мир 3D";
    }
    return "World 3D";
}

static void init_world(void) {
    w_px = 0;
    w_pz = 0;
    w_py = 0;
    w_vy = 0;
    w_face = 0;
    w_walk_t = 0;
    w_moving = 0;
    w_camx = 0;
    w_camz = 0 - WR_CAM_DIST;
    w_time = 0;
    /* Same stick placement and radius as the battle screen. */
    joy.x = 130;
    joy.y = screen_h - 150;
    joy.r = 70;
    w_jump_x = screen_w - 140;
    w_jump_y = screen_h - 150;
    w_jump_id = -1;
    drop_joy();
}

/* Pushes the player out of the toy blocks (circle vs square). */
static void world_collide(void) {
    int i = 0;
    while (i < WR_BLOCKS) {
        double dx = w_px - wb_x[i];
        double dz = w_pz - wb_z[i];
        double r = wb_half[i] + 0.45;
        if (dx > 0 - r && dx < r && dz > 0 - r && dz < r) {
            double push_x = r - fabs(dx);
            double push_z = r - fabs(dz);
            if (push_x < push_z) {
                if (dx >= 0) {
                    w_px = w_px + push_x;
                } else {
                    w_px = w_px - push_x;
                }
            } else {
                if (dz >= 0) {
                    w_pz = w_pz + push_z;
                } else {
                    w_pz = w_pz - push_z;
                }
            }
        }
        i = i + 1;
    }
}

static void update_world(void) {
    /* Same stick speed as the battle, converted to world units. */
    double spd = joy_speed / 50;
    double mx = joy.dx;
    double mz = 0 - joy.dy;
    double mag = sqrt(mx * mx + mz * mz);
    w_time = w_time + dt;
    w_px = w_px + mx * spd * dt;
    w_pz = w_pz + mz * spd * dt;
    if (mag > 0.15) {
        w_face = atan2(mx, mz);
        w_walk_t = w_walk_t + dt * (4 + 6 * mag);
        w_moving = 1;
    } else {
        w_moving = 0;
    }
    if (w_py > 0 || w_vy != 0) {
        w_vy = w_vy - 13 * dt;
        w_py = w_py + w_vy * dt;
        if (w_py <= 0) {
            w_py = 0;
            w_vy = 0;
        }
    }
    world_collide();
    if (w_px < 0 - WR_HALF) {
        w_px = 0 - WR_HALF;
    }
    if (w_px > WR_HALF) {
        w_px = WR_HALF;
    }
    if (w_pz < 0 - WR_HALF) {
        w_pz = 0 - WR_HALF;
    }
    if (w_pz > WR_HALF) {
        w_pz = WR_HALF;
    }
    /* Third-person camera: follows the player from behind and above. */
    w_camx = w_camx + (w_px - w_camx) * clamp(dt * 6, 0, 1);
    w_camz = w_camz + (w_pz - WR_CAM_DIST - w_camz) * clamp(dt * 6, 0, 1);
}

static void touch_world(double x, double y, double action, double id) {
    if (action == 3) {
        w_jump_id = -1;
        drop_joy();
        return;
    }
    if (back_hit(x, y) == 1 && action == 0) {
        play_button_click();
        start_transition(ST_MODES);
        return;
    }
    if (action == 0 && sqrt((x - w_jump_x) * (x - w_jump_x) + (y - w_jump_y) * (y - w_jump_y)) <= 96) {
        w_jump_id = id;
        if (w_py <= 0) {
            w_vy = 4.6;
            w_py = 0.001;
        }
        return;
    }
    if (action == 1 && id == w_jump_id) {
        w_jump_id = -1;
        return;
    }
    joy_touch(x, y, action, id);
}
