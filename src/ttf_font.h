/* ttf_font.h - real font rendering via FreeType.
 *
 * Rasterizes glyphs from a bundled TrueType font (assets/DejaVuSans-Bold.ttf)
 * to 8-bit coverage bitmaps, then blits them through plat_blit_alpha.
 */
#ifndef TTF_FONT_H
#define TTF_FONT_H

#include "platform.h"

/* Loads the font file. Returns 0 on success. */
int ttf_init(const char *font_path);

void ttf_shutdown(void);

/* Draws text with the top-left of its bounding box at virtual (vx, vy),
 * glyph height given in virtual units. Returns the virtual width consumed. */
int ttf_draw_text(int vx, int vy, int v_height, Color c, const char *text);

/* Virtual width a string would occupy at the given glyph height, for
 * centering, without drawing anything. */
int ttf_text_width(int v_height, const char *text);

#endif
