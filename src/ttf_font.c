#include "ttf_font.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>

#include "viewport.h"

static FT_Library g_ft;
static FT_Face g_face;
static int g_ok = 0;

int ttf_init(const char *font_path) {
    if (FT_Init_FreeType(&g_ft)) {
        fprintf(stderr, "ttf_init: FT_Init_FreeType failed\n");
        return -1;
    }
    if (FT_New_Face(g_ft, font_path, 0, &g_face)) {
        fprintf(stderr, "ttf_init: failed to load font '%s'\n", font_path);
        return -1;
    }
    g_ok = 1;
    return 0;
}

void ttf_shutdown(void) {
    if (g_ok) {
        FT_Done_Face(g_face);
        FT_Done_FreeType(g_ft);
    }
}

static void set_pixel_size(int v_height) {
    int real_h = (int)(v_height * viewport_scale());
    if (real_h < 1) real_h = 1;
    FT_Set_Pixel_Sizes(g_face, 0, (unsigned)real_h);
}

int ttf_draw_text(int vx, int vy, int v_height, Color c, const char *text) {
    if (!g_ok) return 0;
    set_pixel_size(v_height);

    int rx, ry;
    viewport_to_real(vx, vy, &rx, &ry);
    int baseline = ry + (int)(g_face->size->metrics.ascender >> 6);
    int pen_x = rx;

    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (FT_Load_Char(g_face, *p, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = g_face->glyph;
        if (g->bitmap.width > 0 && g->bitmap.rows > 0) {
            plat_blit_alpha(pen_x + g->bitmap_left, baseline - g->bitmap_top,
                             (int)g->bitmap.width, (int)g->bitmap.rows,
                             g->bitmap.buffer, c);
        }
        pen_x += (int)(g->advance.x >> 6);
    }

    double scale = viewport_scale();
    return (int)((pen_x - rx) / (scale > 0 ? scale : 1));
}

int ttf_text_width(int v_height, const char *text) {
    if (!g_ok) return 0;
    set_pixel_size(v_height);
    int width_px = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (FT_Load_Char(g_face, *p, FT_LOAD_DEFAULT)) continue;
        width_px += (int)(g_face->glyph->advance.x >> 6);
    }
    double scale = viewport_scale();
    return (int)(width_px / (scale > 0 ? scale : 1));
}
