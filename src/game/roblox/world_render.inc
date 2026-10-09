/* Injoer world rendering: a tiny fixed-camera 3D projection drawn with the
 * existing 2D primitives (rect/circle/ring/line/text). The camera sits
 * behind and above the player and looks along +z; depth only ever grows
 * forward, so a painter's sort by z is enough for correct overlap. */

static double wr_focal(void) { return screen_h * 0.92; }

static double wr_horizon(void) { return screen_h * 0.40; }

static double wr_depth(double z) { return z - w_camz; }

static void wr_project(double x, double y, double z, double *sx, double *sy, double *sc) {
    double rz = wr_depth(z);
    if (rz < 0.6) {
        rz = 0.6;
    }
    *sc = wr_focal() / rz;
    *sx = screen_w / 2 + (x - w_camx) * (*sc);
    *sy = wr_horizon() + (WR_CAM_H - y) * (*sc);
}

static void wr_draw_sky(void) {
    double horizon = wr_horizon();
    double drift = sin(w_time * 0.12) * screen_w * 0.02;
    rect(0, 0, screen_w, horizon, 0x87CEEB);
    circle(screen_w - 120, 90, 58, 0x44FFEB3B);
    circle(screen_w - 120, 90, 42, 0xFFEB3B);
    circle(screen_w * 0.22 + drift, horizon * 0.35, 26, 0xFFFFFF);
    circle(screen_w * 0.22 + drift + 34, horizon * 0.35 + 8, 20, 0xFFFFFF);
    circle(screen_w * 0.60 - drift, horizon * 0.55, 22, 0xFFFFFF);
    circle(screen_w * 0.60 - drift + 30, horizon * 0.55 + 6, 16, 0xFFFFFF);
    rect(0, horizon, screen_w, screen_h - horizon, 0x3AA655);
}

static void wr_draw_grid(void) {
    double gz = floor((w_camz + 2) / 2) * 2;
    double gz_end = w_camz + 44;
    while (gz < gz_end) {
        double x1, y1, s1, x2, y2, s2;
        wr_project(w_camx - 32, 0, gz, &x1, &y1, &s1);
        wr_project(w_camx + 32, 0, gz, &x2, &y2, &s2);
        line(x1, y1, x2, y2, 2, 0x2F8A46);
        gz = gz + 2;
    }
    {
        double gx = floor((w_camx - 26) / 2) * 2;
        double gx_end = w_camx + 26;
        while (gx < gx_end) {
            double x1, y1, s1, x2, y2, s2;
            wr_project(gx, 0, w_camz + 1.5, &x1, &y1, &s1);
            wr_project(gx, 0, w_camz + 44, &x2, &y2, &s2);
            line(x1, y1, x2, y2, 2, 0x2F8A46);
            gx = gx + 2;
        }
    }
}

/* Baseplate studs, like the classic building plate. Only the nearby rows
 * are drawn so the frame stays cheap. */
static void wr_draw_studs(void) {
    double horizon = wr_horizon();
    double gx = floor(w_camx / 4) * 4;
    double gx_end = w_camx + 26;
    while (gx < gx_end) {
        double gz = floor((w_camz + 2) / 4) * 4;
        double gz_end = w_camz + 24;
        while (gz < gz_end) {
            double sx, sy, sc;
            wr_project(gx, 0, gz, &sx, &sy, &sc);
            if (sx > -20 && sx < screen_w + 20 && sy > horizon && sy < screen_h + 20) {
                double r = sc * 0.16;
                if (r < 1.5) {
                    r = 1.5;
                }
                if (r > 9) {
                    r = 9;
                }
                circle(sx, sy, r, 0x46B95C);
            }
            gz = gz + 4;
        }
        gx = gx + 4;
    }
}

static void wr_draw_plate_edge(double ax, double az, double bx, double bz) {
    if (wr_depth(az) < 1 || wr_depth(bz) < 1) {
        return;
    }
    {
        double x1, y1, s1, x2, y2, s2;
        wr_project(ax, 0, az, &x1, &y1, &s1);
        wr_project(bx, 0, bz, &x2, &y2, &s2);
        line(x1, y1, x2, y2, 3, 0xFFD54A);
    }
}

static void wr_draw_plate_border(void) {
    wr_draw_plate_edge(0 - WR_HALF, 0 - WR_HALF, WR_HALF, 0 - WR_HALF);
    wr_draw_plate_edge(WR_HALF, 0 - WR_HALF, WR_HALF, WR_HALF);
    wr_draw_plate_edge(WR_HALF, WR_HALF, 0 - WR_HALF, WR_HALF);
    wr_draw_plate_edge(0 - WR_HALF, WR_HALF, 0 - WR_HALF, 0 - WR_HALF);
}

static void wr_draw_block(double bx, double bz, double half, double h, double col) {
    double sx, sy, sc;
    if (wr_depth(bz) < 1) {
        return;
    }
    wr_project(bx, 0, bz, &sx, &sy, &sc);
    {
        double hw = half * sc;
        double hh = h * sc;
        if (sx + hw < 0 || sx - hw > screen_w) {
            return;
        }
        circle(sx, sy, hw * 1.05, 0x33000000);
        rect(sx - hw, sy - hh, hw * 2, hh, col);
        rect(sx - hw, sy - hh, hw * 2, hh * 0.22, blend_color(col, 0xFFFFFF, 0.35));
        line(sx - hw, sy - hh, sx + hw, sy - hh, 2, shade(col, 0.55));
        line(sx - hw, sy - hh, sx - hw, sy, 2, shade(col, 0.55));
        line(sx + hw, sy - hh, sx + hw, sy, 2, shade(col, 0.55));
    }
}

static void wr_draw_block_by(int i) { wr_draw_block(wb_x[i], wb_z[i], wb_half[i], wb_h[i], wb_c[i]); }

/* Blocky avatar: green legs, blue torso, yellow head and arms. */
static void wr_draw_player(void) {
    double sx, gy, sc;
    wr_project(w_px, 0, w_pz, &sx, &gy, &sc);
    {
        double u = sc;
        double feet, shadow_r, swing, leg_h, leg_w, torso_h, torso_w, torso_y, arm_w;
        double head, head_y, look, eye;
        const char *nick;
        double nsc;
        if (u < 4) {
            u = 4;
        }
        feet = gy - w_py * sc;
        shadow_r = 0.55 * sc * (1 - clamp(w_py / 6, 0, 0.5));
        circle(sx, gy, shadow_r, 0x44000000);
        swing = 0;
        if (w_moving == 1 && w_py <= 0) {
            swing = sin(w_walk_t * 6) * 0.22 * u;
        }
        leg_h = 0.75 * u;
        leg_w = 0.26 * u;
        rect(sx - 0.30 * u, feet - leg_h + swing * 0.4, leg_w, leg_h, 0x02A714);
        rect(sx + 0.30 * u - leg_w, feet - leg_h - swing * 0.4, leg_w, leg_h, 0x02A714);
        torso_h = 0.8 * u;
        torso_w = 0.72 * u;
        torso_y = feet - leg_h - torso_h;
        rect(sx - torso_w / 2, torso_y, torso_w, torso_h, 0x0D69AC);
        arm_w = 0.22 * u;
        rect(sx - torso_w / 2 - arm_w - 0.04 * u, torso_y - swing * 0.4, arm_w, torso_h, 0xF5CD30);
        rect(sx + torso_w / 2 + 0.04 * u, torso_y + swing * 0.4, arm_w, torso_h, 0xF5CD30);
        head = 0.62 * u;
        head_y = torso_y - head - 0.04 * u;
        rect(sx - head / 2, head_y, head, head, 0xF5CD30);
        look = sin(w_face) * 0.10 * u;
        eye = head * 0.13;
        if (eye < 2) {
            eye = 2;
        }
        rect(sx - head * 0.18 + look - eye / 2, head_y + head * 0.30, eye, eye * 1.4, 0x1A1A1A);
        rect(sx + head * 0.18 + look - eye / 2, head_y + head * 0.30, eye, eye * 1.4, 0x1A1A1A);
        line(sx - head * 0.16 + look, head_y + head * 0.68, sx + head * 0.16 + look, head_y + head * 0.68, eye * 0.7,
             0x1A1A1A);
        nick = "Player";
        nsc = sc / 110;
        if (nsc < 0.4) {
            nsc = 0.4;
        }
        if (nsc > 0.8) {
            nsc = 0.8;
        }
        text_scaled(nick, sx - text_ink_width(nick) * nsc / 2, head_y - 30 * nsc, 0xFFFFFF, nsc);
    }
}

static void wr_draw_objects(void) {
    double order[WR_BLOCKS];
    double pd = w_pz - w_camz;
    int drawn_player = 0;
    int i = 0;
    int j = 0;
    while (i < WR_BLOCKS) {
        order[i] = i;
        i = i + 1;
    }
    /* Sort far to near, then draw the player at its own depth. */
    i = 0;
    while (i < WR_BLOCKS) {
        j = i + 1;
        while (j < WR_BLOCKS) {
            if (wb_z[(int)order[j]] > wb_z[(int)order[i]]) {
                double t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    i = 0;
    while (i < WR_BLOCKS) {
        double bd = wb_z[(int)order[i]] - w_camz;
        if (drawn_player == 0 && bd <= pd) {
            wr_draw_player();
            drawn_player = 1;
        }
        wr_draw_block_by((int)order[i]);
        i = i + 1;
    }
    if (drawn_player == 0) {
        wr_draw_player();
    }
}

static void wr_draw_hud(void) {
    const char *sub = "OFFLINE - 3D WORLD";
    text_scaled("INJOER DEBUG", 16, 16, 0xFFFFFF, 0.55);
    if (language == 1) {
        sub = "ОФФЛАЙН - МИР 3D";
    }
    text_scaled(sub, 16, 44, 0xFFFFFF, 0.45);
}

static void wr_draw_controls(void) {
    const char *jl = "JUMP";
    /* The very same stick look as the Cubic Battle fight screen. */
    ring(joy.x, joy.y, joy.r, 4, 0x000000);
    circle(joy.x + joy.ox, joy.y + joy.oy, 34, 0x000000);
    circle(w_jump_x, w_jump_y, 70, 0x7ED321);
    ring(w_jump_x, w_jump_y, 70, 4, 0x000000);
    if (language == 1) {
        jl = "ПРЫГ";
    }
    text_in_box(jl, w_jump_x - 70, w_jump_y - 70, 140, 140, 0xFFFFFF, 0.55);
    draw_back();
}

static void draw_world(void) {
    wr_draw_sky();
    wr_draw_grid();
    wr_draw_studs();
    wr_draw_plate_border();
    wr_draw_objects();
    wr_draw_hud();
    wr_draw_controls();
}
