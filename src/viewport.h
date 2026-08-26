/* viewport.h - resolution/aspect-ratio independence.
 *
 * Game logic and drawing code work entirely in a fixed virtual coordinate
 * space (VIRTUAL_W x VIRTUAL_H). viewport_init() computes a single uniform
 * scale factor from the real screen size (queried via the platform layer)
 * and centers the virtual space with letterboxing/pillarboxing as needed,
 * so shapes never stretch/squish when the display's aspect ratio differs
 * from the virtual space's.
 */
#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "platform.h"

#define VIRTUAL_W 1000
#define VIRTUAL_H 600

void viewport_init(void);

/* Uniform virtual->real scale factor, and the equivalent transform for a
 * single point. Exposed for font.c, which blits pre-rasterized glyph
 * bitmaps directly in real pixels rather than going through vfill_rect. */
double viewport_scale(void);
void viewport_to_real(int vx, int vy, int *rx, int *ry);

/* Fills a rect given in virtual coordinates. */
void vfill_rect(int vx, int vy, int vw, int vh, Color c);

/* Draws a thick line (built from stamped rects) in virtual coordinates. */
void vdraw_thick_line(int vx0, int vy0, int vx1, int vy1, int thickness,
                       Color c);

/* Clears the whole virtual canvas (including letterbox bars) to c. */
void vclear(Color c);

#endif
