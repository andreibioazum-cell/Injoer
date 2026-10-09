/* Touch: left stick with the old battle feel (unit direction past a small
 * deadzone), jump on a tap of the green button, camera orbit on any other
 * drag - a drag started on the jump button becomes an orbit too. */

static void s3_stick_upd(double x, double y) {
    double dx = x - w3_sx;
    double dy = y - w3_sy;
    double d = sqrt(dx * dx + dy * dy);
    double kd = d;
    if (kd > w3_sr) {
        kd = w3_sr;
    }
    if (d > 0) {
        w3_sox = dx * kd / d;
        w3_soy = dy * kd / d;
    } else {
        w3_sox = 0;
        w3_soy = 0;
    }
    if (d > w3_sr * 0.15) {
        w3_sdx = dx / d;
        w3_sdy = dy / d;
    } else {
        w3_sdx = 0;
        w3_sdy = 0;
    }
}

static void s3_stick_reset(void) {
    w3_sid = -1;
    w3_sdx = 0;
    w3_sdy = 0;
    w3_sox = 0;
    w3_soy = 0;
}

static void touch_world(double x, double y, double action, double id) {
    if (action == 3) {
        s3_stick_reset();
        w3_jid = -1;
        w3_oid = -1;
        return;
    }
    if (action == 0) {
        double ds = sqrt((x - w3_sx) * (x - w3_sx) + (y - w3_sy) * (y - w3_sy));
        double dj = sqrt((x - w3_jx) * (x - w3_jx) + (y - w3_jy) * (y - w3_jy));
        if (w3_sid == -1 && ds <= 150) {
            w3_sid = id;
            s3_stick_upd(x, y);
            return;
        }
        if (w3_jid == -1 && w3_oid == -1 && dj <= 96) {
            w3_jid = id;
            w3_jx0 = x;
            w3_jy0 = y;
            w3_jmoved = 0;
            return;
        }
        if (w3_oid == -1) {
            w3_oid = id;
            w3_olx = x;
            w3_oly = y;
        }
        return;
    }
    if (action == 2) {
        if (id == w3_sid) {
            s3_stick_upd(x, y);
            return;
        }
        if (id == w3_jid) {
            double moved = sqrt((x - w3_jx0) * (x - w3_jx0) + (y - w3_jy0) * (y - w3_jy0));
            if (moved > 28) {
                w3_jmoved = 1;
                w3_jid = -1;
                if (w3_oid == -1) {
                    w3_oid = id;
                    w3_olx = x;
                    w3_oly = y;
                }
            }
            return;
        }
        if (id == w3_oid) {
            w3_cam_yaw = w3_cam_yaw - (x - w3_olx) * 0.008;
            w3_cam_pitch = w3_cam_pitch + (y - w3_oly) * 0.006;
            if (w3_cam_pitch < 0.12) {
                w3_cam_pitch = 0.12;
            }
            if (w3_cam_pitch > 1.15) {
                w3_cam_pitch = 1.15;
            }
            w3_olx = x;
            w3_oly = y;
        }
        return;
    }
    if (action == 1) {
        if (id == w3_sid) {
            s3_stick_reset();
            return;
        }
        if (id == w3_jid) {
            if (w3_jmoved == 0) {
                world_jump();
            }
            w3_jid = -1;
            return;
        }
        if (id == w3_oid) {
            w3_oid = -1;
        }
    }
}
