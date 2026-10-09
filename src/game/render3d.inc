/* Injoer 3D renderer. True perspective: world-space boxes are backface-culled,
 * clipped against the near plane in view space, projected and emitted as
 * flat-shaded triangles, far boxes first (no depth buffer on the 2D pipe). */

#define R3_FOV 1.0471975511965976
#define R3_NEAR 0.15
#define R3_FOG_NEAR 25.0
#define R3_FOG_FAR 90.0
#define R3_POOL 256
#define R3_SKY 0x87CEEB
#define R3_GROUND_A 0x3AA655
#define R3_GROUND_B 0x349E4E

typedef struct {
    V3 c[8];
    V3 n[6];
    double depth;
    double color;
} R3Box;

/* Corner i: x = bit0 ? +hx : -hx, y = bit1 ? +hy : -hy, z = bit2 ? +hz : -hz.
 * Faces: +X, -X, +Y, -Y, +Z, -Z. */
static const int r3_face[6][4] = {{1, 3, 7, 5}, {0, 2, 6, 4}, {2, 3, 7, 6},
                                  {0, 1, 5, 4}, {4, 5, 7, 6}, {0, 1, 3, 2}};
static const V3 r3_sun = {0.45, 0.75, 0.5};

static R3Box r3_pool[R3_POOL];
static int r3_n;
static M4 r3_view;
static V3 r3_cam;
static double r3_f;
static double r3_cx;
static double r3_cy;

static double r3_shade(double rgb, double k) {
    double a = floor(rgb / 16777216);
    double body = rgb - a * 16777216;
    double r = floor(body / 65536);
    double g = floor(body / 256) - r * 256;
    double b = body - floor(body / 256) * 256;
    r = floor(r * k + 0.5);
    g = floor(g * k + 0.5);
    b = floor(b * k + 0.5);
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    return a * 16777216 + r * 65536 + g * 256 + b;
}

static double r3_mix(double ca, double cb, double t) {
    double ba = ca - floor(ca / 16777216) * 16777216;
    double bb = cb - floor(cb / 16777216) * 16777216;
    double ar = floor(ba / 65536);
    double ag = floor(ba / 256) - ar * 256;
    double ab = ba - floor(ba / 256) * 256;
    double br = floor(bb / 65536);
    double bg = floor(bb / 256) - br * 256;
    double bbb = bb - floor(bb / 256) * 256;
    double r = floor(ar + (br - ar) * t + 0.5);
    double g = floor(ag + (bg - ag) * t + 0.5);
    double b = floor(ab + (bbb - ab) * t + 0.5);
    return floor(ca / 16777216) * 16777216 + r * 65536 + g * 256 + b;
}

static void r3_begin(void) {
    w3_cam_view(&r3_view, &r3_cam);
    r3_f = (screen_h * 0.5) / tan(R3_FOV * 0.5);
    r3_cx = screen_w * 0.5;
    r3_cy = screen_h * 0.5;
    r3_n = 0;
}

static void r3_proj(double x, double y, double z, double *sx, double *sy) {
    double s = r3_f / (0 - z);
    *sx = r3_cx + x * s;
    *sy = r3_cy - y * s;
}

/* One triangle in view coords: clip vs the near plane, project, emit. */
static void r3_tri(double ax, double ay, double az, double bx, double by, double bz, double cx, double cy,
                   double cz, double color) {
    double px[8], py[8], pz[8];
    double qx[8], qy[8], qz[8];
    double sx[8], sy[8];
    int n = 3;
    int i = 0;
    int j = 0;
    int k;
    px[0] = ax;
    py[0] = ay;
    pz[0] = az;
    px[1] = bx;
    py[1] = by;
    pz[1] = bz;
    px[2] = cx;
    py[2] = cy;
    pz[2] = cz;
    /* Sutherland-Hodgman against z <= -R3_NEAR. */
    while (i < n) {
        k = (i + 1) % n;
        {
            double zi = pz[i] + R3_NEAR;
            double zk = pz[k] + R3_NEAR;
            int in_i = zi <= 0;
            int in_k = zk <= 0;
            if (in_i && in_k) {
                qx[j] = px[k];
                qy[j] = py[k];
                qz[j] = pz[k];
                j = j + 1;
            } else if (in_i) {
                double t = zi / (zi - zk);
                qx[j] = px[i] + (px[k] - px[i]) * t;
                qy[j] = py[i] + (py[k] - py[i]) * t;
                qz[j] = 0 - R3_NEAR;
                j = j + 1;
            } else if (in_k) {
                double t = zi / (zi - zk);
                qx[j] = px[i] + (px[k] - px[i]) * t;
                qy[j] = py[i] + (py[k] - py[i]) * t;
                qz[j] = 0 - R3_NEAR;
                j = j + 1;
                qx[j] = px[k];
                qy[j] = py[k];
                qz[j] = pz[k];
                j = j + 1;
            }
        }
        i = i + 1;
    }
    n = j;
    i = 0;
    while (i < n) {
        r3_proj(qx[i], qy[i], qz[i], &sx[i], &sy[i]);
        i = i + 1;
    }
    i = 1;
    while (i + 1 < n) {
        tri(sx[0], sy[0], sx[i], sy[i], sx[i + 1], sy[i + 1], color);
        i = i + 1;
    }
}

static void r3_quad(double ax, double ay, double az, double bx, double by, double bz, double cx, double cy,
                    double cz, double dx, double dy, double dz, double color) {
    r3_tri(ax, ay, az, bx, by, bz, cx, cy, cz, color);
    r3_tri(ax, ay, az, cx, cy, cz, dx, dy, dz, color);
}

static void r3_collect(const M4 *m, double hx, double hy, double hz, double color) {
    R3Box *b;
    V3 center;
    V3 vc;
    int i;
    if (r3_n >= R3_POOL) {
        return;
    }
    b = &r3_pool[r3_n];
    i = 0;
    while (i < 8) {
        V3 p = m3_v(i & 1 ? hx : 0 - hx, i & 2 ? hy : 0 - hy, i & 4 ? hz : 0 - hz);
        b->c[i] = m3_pt(m, p);
        i = i + 1;
    }
    /* Face normals: the matrix has no scale, so plain rotation is enough. */
    b->n[0] = m3_dir(m, m3_v(1, 0, 0));
    b->n[1] = m3_dir(m, m3_v(-1, 0, 0));
    b->n[2] = m3_dir(m, m3_v(0, 1, 0));
    b->n[3] = m3_dir(m, m3_v(0, -1, 0));
    b->n[4] = m3_dir(m, m3_v(0, 0, 1));
    b->n[5] = m3_dir(m, m3_v(0, 0, -1));
    center = m3_pt(m, m3_v(0, 0, 0));
    vc = m3_pt(&r3_view, center);
    b->depth = 0 - vc.z;
    b->color = color;
    r3_n = r3_n + 1;
}

static void r3_emit_box(R3Box *b) {
    int f = 0;
    double fog = clamp((b->depth - R3_FOG_NEAR) / (R3_FOG_FAR - R3_FOG_NEAR), 0, 1);
    while (f < 6) {
        V3 n = b->n[f];
        V3 p0 = b->c[r3_face[f][0]];
        V3 p1 = b->c[r3_face[f][1]];
        V3 p2 = b->c[r3_face[f][2]];
        V3 p3 = b->c[r3_face[f][3]];
        V3 mid = m3_v((p0.x + p1.x + p2.x + p3.x) * 0.25, (p0.y + p1.y + p2.y + p3.y) * 0.25,
                      (p0.z + p1.z + p2.z + p3.z) * 0.25);
        double side = n.x * (mid.x - r3_cam.x) + n.y * (mid.y - r3_cam.y) + n.z * (mid.z - r3_cam.z);
        if (side < 0) {
            double nl = n.x * r3_sun.x + n.y * r3_sun.y + n.z * r3_sun.z;
            double col;
            V3 v0, v1, v2, v3;
            if (nl < 0) {
                nl = 0;
            }
            col = r3_mix(r3_shade(b->color, 0.55 + 0.45 * nl), R3_SKY, fog);
            v0 = m3_pt(&r3_view, p0);
            v1 = m3_pt(&r3_view, p1);
            v2 = m3_pt(&r3_view, p2);
            v3 = m3_pt(&r3_view, p3);
            r3_quad(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, col);
        }
        f = f + 1;
    }
}

static void r3_flush(void) {
    int i = 1;
    /* Boxes barely overlap, so a far-to-near sort of whole boxes is enough. */
    while (i < r3_n) {
        R3Box t = r3_pool[i];
        int j = i - 1;
        while (j >= 0 && r3_pool[j].depth < t.depth) {
            r3_pool[j + 1] = r3_pool[j];
            j = j - 1;
        }
        r3_pool[j + 1] = t;
        i = i + 1;
    }
    i = 0;
    while (i < r3_n) {
        r3_emit_box(&r3_pool[i]);
        i = i + 1;
    }
    r3_n = 0;
}

/* Checker ground fading into the sky, drawn before everything else. */
static void r3_ground(void) {
    double step = 220.0 / 12;
    int gx = 0;
    while (gx < 12) {
        int gz = 0;
        while (gz < 12) {
            double x0 = 0 - 110.0 + gx * step;
            double z0 = 0 - 110.0 + gz * step;
            V3 v0 = m3_pt(&r3_view, m3_v(x0, 0, z0));
            V3 v1 = m3_pt(&r3_view, m3_v(x0 + step, 0, z0));
            V3 v2 = m3_pt(&r3_view, m3_v(x0 + step, 0, z0 + step));
            V3 v3 = m3_pt(&r3_view, m3_v(x0, 0, z0 + step));
            double dist = 0 - (v0.z + v1.z + v2.z + v3.z) * 0.25;
            double fog = clamp((dist - R3_FOG_NEAR) / (R3_FOG_FAR - R3_FOG_NEAR), 0, 1);
            double col = (gx + gz) % 2 == 0 ? R3_GROUND_A : R3_GROUND_B;
            r3_quad(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z,
                    r3_mix(col, R3_SKY, fog));
            gz = gz + 1;
        }
        gx = gx + 1;
    }
}

static void r3_shadow(void) {
    double s = 0.7 * (1 - clamp(w3_py / 6, 0, 0.5));
    V3 v0 = m3_pt(&r3_view, m3_v(w3_px - s, 0.03, w3_pz - s));
    V3 v1 = m3_pt(&r3_view, m3_v(w3_px + s, 0.03, w3_pz - s));
    V3 v2 = m3_pt(&r3_view, m3_v(w3_px + s, 0.03, w3_pz + s));
    V3 v3 = m3_pt(&r3_view, m3_v(w3_px - s, 0.03, w3_pz + s));
    r3_quad(v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 0x55000000);
}

static void r3_blocks(void) {
    int i = 0;
    while (i < W3_BLOCKS) {
        M4 m;
        m3_translate(&m, w3_bx[i], w3_bh[i] * 0.5, w3_bz[i]);
        r3_collect(&m, w3_bhx[i], w3_bh[i] * 0.5, w3_bhz[i], w3_bc[i]);
        i = i + 1;
    }
}

static void r3_walls(void) {
    M4 m;
    m3_translate(&m, 0, 0.5, 0 - W3_LIMIT);
    r3_collect(&m, W3_LIMIT + 0.5, 0.5, 0.4, 0xFFD54A);
    m3_translate(&m, 0, 0.5, W3_LIMIT);
    r3_collect(&m, W3_LIMIT + 0.5, 0.5, 0.4, 0xFFD54A);
    m3_translate(&m, 0 - W3_LIMIT, 0.5, 0);
    r3_collect(&m, 0.4, 0.5, W3_LIMIT + 0.5, 0xFFD54A);
    m3_translate(&m, W3_LIMIT, 0.5, 0);
    r3_collect(&m, 0.4, 0.5, W3_LIMIT + 0.5, 0xFFD54A);
}

static void r3_studs(void) {
    int ix = 0;
    while (ix < W3_STUD_N) {
        int iz = 0;
        while (iz < W3_STUD_N) {
            M4 m;
            double x = (ix - (W3_STUD_N - 1) * 0.5) * W3_STUD_GAP;
            double z = (iz - (W3_STUD_N - 1) * 0.5) * W3_STUD_GAP;
            m3_translate(&m, x, 0.2, z);
            r3_collect(&m, 0.7, 0.2, 0.7, 0x46B95C);
            iz = iz + 1;
        }
        ix = ix + 1;
    }
}

/* Blocky avatar out of real boxes: legs swing at the hips, arms at the
 * shoulders, the head carries the face. */
static void r3_player(void) {
    M4 base, m, t1, t2;
    double sw = 0;
    if (w3_moving == 1 && w3_py <= 0) {
        sw = sin(w3_walk * 6) * 0.5;
    }
    m3_translate(&t1, w3_px, w3_py, w3_pz);
    m3_rot_y(&t2, w3_face);
    m3_mul(&base, &t1, &t2);
    /* Torso. */
    m3_translate(&t1, 0, 1.22, 0);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.40, 0.43, 0.23, 0x0D69AC);
    /* Head. */
    m3_translate(&t1, 0, 1.95, 0);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.30, 0.30, 0.30, 0xF5CD30);
    /* Eyes and mouth, slightly proud of the face. */
    m3_translate(&t1, -0.12, 2.02, 0.27);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.05, 0.07, 0.04, 0x1A1A1A);
    m3_translate(&t1, 0.12, 2.02, 0.27);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.05, 0.07, 0.04, 0x1A1A1A);
    m3_translate(&t1, 0, 1.88, 0.28);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.09, 0.025, 0.03, 0x1A1A1A);
    /* Legs: pivot at the hip, geometry hangs below it. */
    m3_translate(&t1, -0.21, 0.8, 0);
    m3_rot_x(&t2, sw);
    m3_mul(&t1, &t1, &t2);
    m3_translate(&t2, 0, -0.4, 0);
    m3_mul(&t1, &t1, &t2);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.18, 0.4, 0.2, 0x02A714);
    m3_translate(&t1, 0.21, 0.8, 0);
    m3_rot_x(&t2, 0 - sw);
    m3_mul(&t1, &t1, &t2);
    m3_translate(&t2, 0, -0.4, 0);
    m3_mul(&t1, &t1, &t2);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.18, 0.4, 0.2, 0x02A714);
    /* Arms swing against the legs. */
    m3_translate(&t1, -0.55, 1.55, 0);
    m3_rot_x(&t2, 0 - sw * 0.8);
    m3_mul(&t1, &t1, &t2);
    m3_translate(&t2, 0, -0.35, 0);
    m3_mul(&t1, &t1, &t2);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.14, 0.35, 0.16, 0xF5CD30);
    m3_translate(&t1, 0.55, 1.55, 0);
    m3_rot_x(&t2, sw * 0.8);
    m3_mul(&t1, &t1, &t2);
    m3_translate(&t2, 0, -0.35, 0);
    m3_mul(&t1, &t1, &t2);
    m3_mul(&m, &base, &t1);
    r3_collect(&m, 0.14, 0.35, 0.16, 0xF5CD30);
}

static void r3_label(const char *s, double cx, double y, double sc, uint32_t col) {
    text_scaled(s, cx - text_ink_width(s) * sc / 2, y, col, sc);
}

static void r3_hud(void) {
    text_scaled("INJOER", 16, 14, 0xFFFFFF, 0.7);
    text_scaled("DEBUG - OFFLINE", 16, 52, 0xFFFFFF, 0.42);
    text_scaled("стик - идти | кнопка - прыжок | тяни - камера", 16, 76, 0xFFFFFF, 0.42);
    /* Same stick look as the old battle screen. */
    ring(w3_sx, w3_sy, w3_sr, 4, 0x000000);
    circle(w3_sx + w3_sox, w3_sy + w3_soy, 34, 0x000000);
    circle(w3_jx, w3_jy, 70, 0x7ED321);
    ring(w3_jx, w3_jy, 70, 4, 0x000000);
    r3_label("ПРЫГ", w3_jx, w3_jy - 14, 0.55, 0xFFFFFF);
}

static void draw_world(void) {
    clear_screen(R3_SKY);
    r3_begin();
    r3_ground();
    r3_shadow();
    r3_blocks();
    r3_walls();
    r3_studs();
    r3_player();
    r3_flush();
    r3_hud();
}
