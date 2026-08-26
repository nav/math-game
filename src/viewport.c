#include "viewport.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static double g_scale = 1.0;
static double g_off_x = 0.0;
static double g_off_y = 0.0;
static int g_screen_w = 0;
static int g_screen_h = 0;

void viewport_init(void) {
    plat_get_screen_size(&g_screen_w, &g_screen_h);
    if (g_screen_w <= 0 || g_screen_h <= 0) {
        fprintf(stderr, "viewport_init: bad screen size %dx%d\n", g_screen_w,
                g_screen_h);
        exit(1);
    }
    double scale_x = (double)g_screen_w / VIRTUAL_W;
    double scale_y = (double)g_screen_h / VIRTUAL_H;
    g_scale = scale_x < scale_y ? scale_x : scale_y;
    g_off_x = (g_screen_w - VIRTUAL_W * g_scale) / 2.0;
    g_off_y = (g_screen_h - VIRTUAL_H * g_scale) / 2.0;
}

double viewport_scale(void) { return g_scale; }

void viewport_to_real(int vx, int vy, int *rx, int *ry) {
    *rx = (int)lround(g_off_x + vx * g_scale);
    *ry = (int)lround(g_off_y + vy * g_scale);
}

void vfill_rect(int vx, int vy, int vw, int vh, Color c) {
    int rx = (int)lround(g_off_x + vx * g_scale);
    int ry = (int)lround(g_off_y + vy * g_scale);
    int rw = (int)lround(vw * g_scale);
    int rh = (int)lround(vh * g_scale);
    if (rw < 1) rw = 1;
    if (rh < 1) rh = 1;
    plat_fill_rect_px(rx, ry, rw, rh, c);
}

void vdraw_thick_line(int vx0, int vy0, int vx1, int vy1, int thickness,
                       Color c) {
    double dx = vx1 - vx0;
    double dy = vy1 - vy0;
    double dist = sqrt(dx * dx + dy * dy);
    double step = thickness * 0.5;
    if (step < 1.0) step = 1.0;
    int steps = (int)(dist / step) + 1;
    for (int i = 0; i <= steps; i++) {
        double t = (double)i / steps;
        int x = (int)lround(vx0 + dx * t);
        int y = (int)lround(vy0 + dy * t);
        vfill_rect(x - thickness / 2, y - thickness / 2, thickness,
                   thickness, c);
    }
}

void vclear(Color c) {
    /* Fill the real screen (including any letterbox bars), not just the
     * virtual canvas, so nothing stale lingers outside it. */
    plat_fill_rect_px(0, 0, g_screen_w, g_screen_h, c);
}
