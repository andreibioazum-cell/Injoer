/* Injoer world: player, orbit camera, scene layout and update.
 * No menus - the game boots straight into the 3D sandbox. */

#define W3_BLOCKS 10
#define W3_STUD_N 10
#define W3_STUD_GAP 4.0
#define W3_LIMIT 22.0
#define W3_SPEED 4.4
#define W3_JUMP_V 4.8
#define W3_GRAVITY 13.0

static double w3_px;
static double w3_pz;
static double w3_py;
static double w3_vy;
static double w3_face;
static double w3_walk;
static double w3_moving;
static double w3_time;
static double w3_cam_yaw;
static double w3_cam_pitch;
static double w3_cam_dist;
static double w3_sx;
static double w3_sy;
static double w3_sr;
static double w3_sdx;
static double w3_sdy;
static double w3_sox;
static double w3_soy;
static double w3_sid;
static double w3_jx;
static double w3_jy;
static double w3_jid;
static double w3_jx0;
static double w3_jy0;
static double w3_jmoved;
static double w3_oid;
static double w3_olx;
static double w3_oly;
static double w3_snd;

/* Toy blocks: center x/z, half extents, height, color. */
static double w3_bx[W3_BLOCKS] = {6, -7, 5, -6, 0, 11, -11, 3, -2, 9};
static double w3_bz[W3_BLOCKS] = {7, 5, -7, -8, 12, -1, -2, 16, -13, 10};
static double w3_bhx[W3_BLOCKS] = {1.6, 2.2, 1.4, 2.0, 1.2, 2.4, 1.8, 1.5, 2.0, 1.3};
static double w3_bhz[W3_BLOCKS] = {1.6, 1.8, 2.2, 1.4, 1.2, 1.6, 2.0, 2.4, 1.5, 1.7};
static double w3_bh[W3_BLOCKS] = {1.6, 2.6, 3.2, 1.2, 2.0, 1.4, 3.6, 2.2, 2.8, 1.0};
static double w3_bc[W3_BLOCKS] = {0xE74C3C, 0xF1C40F, 0x3498DB, 0x9B59B6, 0xE67E22,
                                  0x1ABC9C, 0xE91E63, 0x7ED321, 0xFF7043, 0x26C6DA};

static void world_init(void) {
    w3_px = 0;
    w3_pz = 0;
    w3_py = 0;
    w3_vy = 0;
    w3_face = M3_PI;
    w3_walk = 0;
    w3_moving = 0;
    w3_time = 0;
    w3_cam_yaw = M3_PI;
    w3_cam_pitch = 0.42;
    w3_cam_dist = 11;
    w3_sx = 130;
    w3_sy = screen_h - 150;
    w3_sr = 70;
    w3_sdx = 0;
    w3_sdy = 0;
    w3_sox = 0;
    w3_soy = 0;
    w3_sid = -1;
    w3_jx = screen_w - 140;
    w3_jy = screen_h - 150;
    w3_jid = -1;
    w3_jx0 = 0;
    w3_jy0 = 0;
    w3_jmoved = 0;
    w3_oid = -1;
    w3_olx = 0;
    w3_oly = 0;
    w3_snd = snd_load("jump.wav");
}

/* Third-person orbit camera around the player. */
static void w3_cam_view(M4 *view, V3 *cam) {
    double cp = cos(w3_cam_pitch);
    V3 target;
    cam->x = w3_px + w3_cam_dist * sin(w3_cam_yaw) * cp;
    cam->y = 1.6 + w3_cam_dist * sin(w3_cam_pitch);
    cam->z = w3_pz + w3_cam_dist * cos(w3_cam_yaw) * cp;
    target = m3_v(w3_px, 1.6, w3_pz);
    m3_look_at(view, *cam, target);
}

/* Pushes the player out of the toy blocks (circle vs square). */
static void world_collide(void) {
    int i = 0;
    while (i < W3_BLOCKS) {
        double dx = w3_px - w3_bx[i];
        double dz = w3_pz - w3_bz[i];
        double rx = w3_bhx[i] + 0.45;
        double rz = w3_bhz[i] + 0.45;
        if (dx > 0 - rx && dx < rx && dz > 0 - rz && dz < rz) {
            double push_x = rx - fabs(dx);
            double push_z = rz - fabs(dz);
            if (push_x < push_z) {
                if (dx >= 0) {
                    w3_px = w3_px + push_x;
                } else {
                    w3_px = w3_px - push_x;
                }
            } else {
                if (dz >= 0) {
                    w3_pz = w3_pz + push_z;
                } else {
                    w3_pz = w3_pz - push_z;
                }
            }
        }
        i = i + 1;
    }
}

static void world_update(void) {
    /* Camera-relative walk: stick up is away from the camera. */
    double fx = 0 - sin(w3_cam_yaw);
    double fz = 0 - cos(w3_cam_yaw);
    double rx = 0 - fz;
    double rz = fx;
    double fwd = 0 - w3_sdy;
    double mx = fx * fwd + rx * w3_sdx;
    double mz = fz * fwd + rz * w3_sdx;
    double mag = sqrt(mx * mx + mz * mz);
    w3_time = w3_time + dt;
    w3_px = w3_px + mx * W3_SPEED * dt;
    w3_pz = w3_pz + mz * W3_SPEED * dt;
    if (mag > 0.15) {
        w3_face = atan2(mx, mz);
        w3_walk = w3_walk + dt * (4 + 6 * mag);
        w3_moving = 1;
    } else {
        w3_moving = 0;
    }
    if (w3_py > 0 || w3_vy != 0) {
        w3_vy = w3_vy - W3_GRAVITY * dt;
        w3_py = w3_py + w3_vy * dt;
        if (w3_py <= 0) {
            w3_py = 0;
            w3_vy = 0;
        }
    }
    world_collide();
    if (w3_px < 0 - W3_LIMIT) {
        w3_px = 0 - W3_LIMIT;
    }
    if (w3_px > W3_LIMIT) {
        w3_px = W3_LIMIT;
    }
    if (w3_pz < 0 - W3_LIMIT) {
        w3_pz = 0 - W3_LIMIT;
    }
    if (w3_pz > W3_LIMIT) {
        w3_pz = W3_LIMIT;
    }
}

static void world_jump(void) {
    if (w3_py > 0) {
        return;
    }
    w3_vy = W3_JUMP_V;
    w3_py = 0.001;
    if (w3_snd == 1) {
        snd_play("jump.wav");
    }
}
