/* platform.h - thin interface the game logic drives.
 *
 * Two implementations exist: platform_fbdev.c (Raspberry Pi, raw
 * /dev/fb0 + evdev) and platform_sdl.c (macOS, local dev iteration only).
 * Both deal purely in real screen pixels; virtual-space scaling for
 * resolution independence lives in viewport.h/.c, one layer up.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} Color;

/* GK_ prefix (not KEY_) deliberately: linux/input.h already #defines
 * KEY_0, KEY_ENTER, KEY_BACKSPACE etc. as numeric keycodes, which would
 * collide with enum identifiers of the same name. */
typedef enum {
    GK_NONE = 0,
    GK_0, GK_1, GK_2, GK_3, GK_4,
    GK_5, GK_6, GK_7, GK_8, GK_9,
    GK_ENTER,
    GK_BACKSPACE,
    GK_ESCAPE,
} Key;

/* Sets up the display/input devices. Returns 0 on success. */
int plat_init(void);

void plat_shutdown(void);

/* Real pixel dimensions of the display. */
void plat_get_screen_size(int *w, int *h);

/* Fills an axis-aligned rect in real pixel coordinates. */
void plat_fill_rect_px(int x, int y, int w, int h, Color c);

/* Alpha-blends an 8-bit coverage bitmap (row-major, w*h bytes, one byte
 * per pixel) tinted color c onto the screen at real pixel (x, y). Used
 * for FreeType glyph rendering. */
void plat_blit_alpha(int x, int y, int w, int h, const unsigned char *alpha,
                      Color c);

/* Pushes the drawn frame to the screen. */
void plat_present(void);

/* Non-blocking: returns GK_NONE if nothing new happened. */
Key plat_poll_key(void);

/* Monotonic-ish milliseconds, for feedback-duration timing. */
long plat_time_ms(void);

#endif
