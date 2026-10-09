/* Raw screen-space triangle. Used by CPU-projected 3D: the game does its own
 * perspective divide and clipping, then feeds flat-shaded triangles into the
 * solid (blended) pipeline. Vulkan culling is disabled, so any winding works. */
static int geo_tri(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t c) {
    size_t base;
    if (!isfinite(x1 + y1 + x2 + y2 + x3 + y3))
        return 1;
    /* Skip degenerate (zero-area) triangles. */
    if ((x2 - x1) * (y3 - y1) == (x3 - x1) * (y2 - y1))
        return 1;
    base = geo_vn;
    if (!geo_push_vert(x1, y1, 0, 0, c))
        return 0;
    if (!geo_push_vert(x2, y2, 0, 0, c))
        return 0;
    if (!geo_push_vert(x3, y3, 0, 0, c))
        return 0;
    if (!geo_push_index((uint16_t)base) || !geo_push_index((uint16_t)(base + 1)) ||
        !geo_push_index((uint16_t)(base + 2)))
        return 0;
    return 1;
}

void tri(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t c) {
    DSCmd *p = push(DS_CMD_TRI);
    if (!p)
        return;
    p->v.tr.x1 = x1;
    p->v.tr.y1 = y1;
    p->v.tr.x2 = x2;
    p->v.tr.y2 = y2;
    p->v.tr.x3 = x3;
    p->v.tr.y3 = y3;
    p->v.tr.c = pack_c(c);
}
