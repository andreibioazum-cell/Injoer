/* Injoer 3D math. Row-major 4x4 matrices, column vectors (world = M * local).
 * World space: +x right, +y up, +z toward the default camera side.
 * View space: the camera sits at the origin and looks down -z. */

#define M3_PI 3.141592653589793

typedef struct {
    double x, y, z;
} V3;

typedef struct {
    double m[16];
} M4;

static V3 m3_v(double x, double y, double z) {
    V3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static void m3_identity(M4 *o) {
    int i = 0;
    while (i < 16) {
        o->m[i] = 0;
        i = i + 1;
    }
    o->m[0] = 1;
    o->m[5] = 1;
    o->m[10] = 1;
    o->m[15] = 1;
}

static void m3_mul(M4 *o, const M4 *a, const M4 *b) {
    M4 t;
    int r = 0;
    while (r < 4) {
        int c = 0;
        while (c < 4) {
            t.m[r * 4 + c] = a->m[r * 4] * b->m[c] + a->m[r * 4 + 1] * b->m[4 + c] +
                             a->m[r * 4 + 2] * b->m[8 + c] + a->m[r * 4 + 3] * b->m[12 + c];
            c = c + 1;
        }
        r = r + 1;
    }
    *o = t;
}

static void m3_translate(M4 *o, double x, double y, double z) {
    m3_identity(o);
    o->m[3] = x;
    o->m[7] = y;
    o->m[11] = z;
}

static void m3_rot_x(M4 *o, double a) {
    double c = cos(a);
    double s = sin(a);
    m3_identity(o);
    o->m[5] = c;
    o->m[6] = 0 - s;
    o->m[9] = s;
    o->m[10] = c;
}

static void m3_rot_y(M4 *o, double a) {
    double c = cos(a);
    double s = sin(a);
    m3_identity(o);
    o->m[0] = c;
    o->m[2] = s;
    o->m[8] = 0 - s;
    o->m[10] = c;
}

static V3 m3_pt(const M4 *m, V3 p) {
    V3 o;
    o.x = m->m[0] * p.x + m->m[1] * p.y + m->m[2] * p.z + m->m[3];
    o.y = m->m[4] * p.x + m->m[5] * p.y + m->m[6] * p.z + m->m[7];
    o.z = m->m[8] * p.x + m->m[9] * p.y + m->m[10] * p.z + m->m[11];
    return o;
}

static V3 m3_dir(const M4 *m, V3 d) {
    V3 o;
    o.x = m->m[0] * d.x + m->m[1] * d.y + m->m[2] * d.z;
    o.y = m->m[4] * d.x + m->m[5] * d.y + m->m[6] * d.z;
    o.z = m->m[8] * d.x + m->m[9] * d.y + m->m[10] * d.z;
    return o;
}

static void m3_look_at(M4 *o, V3 eye, V3 target) {
    V3 f = m3_v(target.x - eye.x, target.y - eye.y, target.z - eye.z);
    double fl = sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
    V3 s;
    V3 u;
    if (fl < 0.000001) {
        m3_identity(o);
        return;
    }
    f.x = f.x / fl;
    f.y = f.y / fl;
    f.z = f.z / fl;
    /* side = f x up(0,1,0), up vector = s x f */
    s = m3_v(0 - f.z, 0, f.x);
    {
        double sl = sqrt(s.x * s.x + s.z * s.z);
        if (sl < 0.000001) {
            s = m3_v(1, 0, 0);
        } else {
            s.x = s.x / sl;
            s.z = s.z / sl;
        }
    }
    u = m3_v(s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x);
    o->m[0] = s.x;
    o->m[1] = s.y;
    o->m[2] = s.z;
    o->m[3] = 0 - (s.x * eye.x + s.y * eye.y + s.z * eye.z);
    o->m[4] = u.x;
    o->m[5] = u.y;
    o->m[6] = u.z;
    o->m[7] = 0 - (u.x * eye.x + u.y * eye.y + u.z * eye.z);
    o->m[8] = 0 - f.x;
    o->m[9] = 0 - f.y;
    o->m[10] = 0 - f.z;
    o->m[11] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    o->m[12] = 0;
    o->m[13] = 0;
    o->m[14] = 0;
    o->m[15] = 1;
}
