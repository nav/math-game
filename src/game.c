/* game.c - shared game logic and main loop.
 *
 * Platform-independent: only talks to the plat_ and v-prefixed functions
 * declared in platform.h / viewport.h, so the exact same file builds
 * against either
 * platform_fbdev.c (Pi) or platform_sdl.c (macOS dev loop).
 *
 * Single-digit addition only for now (each operand 0-9).
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"
#include "ttf_font.h"
#include "viewport.h"

#ifndef FONT_PATH
#define FONT_PATH "../assets/DejaVuSans-Bold.ttf"
#endif

static const Color COLOR_BG = {20, 20, 45};
static const Color COLOR_TEXT = {235, 235, 240};
static const Color COLOR_CORRECT = {40, 200, 90};
static const Color COLOR_WRONG = {220, 60, 60};

#define FEEDBACK_DURATION_MS 1200

#define EQ_Y 130
#define EQ_HEIGHT 140

/* Typed answer, shown larger since it's the primary interactive element. */
#define ANS_Y 340
#define ANS_HEIGHT 200

typedef enum { STATE_PROBLEM, STATE_FEEDBACK } GameState;

static void generate_problem(int *a, int *b) {
    *a = rand() % 10;
    *b = rand() % 10;
}

static int key_to_digit(Key k) {
    if (k >= GK_0 && k <= GK_9) return k - GK_0;
    return -1;
}

static void draw_centered(int cy, int v_height, Color c, const char *text) {
    int w = ttf_text_width(v_height, text);
    ttf_draw_text((VIRTUAL_W - w) / 2, cy, v_height, c, text);
}

int main(void) {
    if (plat_init() != 0) {
        fprintf(stderr, "plat_init failed\n");
        return 1;
    }
    viewport_init();
    if (ttf_init(FONT_PATH) != 0) {
        fprintf(stderr, "ttf_init failed for '%s'\n", FONT_PATH);
        return 1;
    }
    srand((unsigned)time(NULL));

    GameState state = STATE_PROBLEM;
    char answer_buf[3] = {0};
    int a, b;
    generate_problem(&a, &b);
    int last_correct = 0;
    long feedback_start_ms = 0;
    int running = 1;
    int dirty = 1; /* draw the first frame */

    while (running) {
        Key k = plat_poll_key();

        if (k == GK_ESCAPE) {
            running = 0;
        } else if (state == STATE_PROBLEM) {
            int d = key_to_digit(k);
            if (d >= 0 && strlen(answer_buf) < 2) {
                size_t n = strlen(answer_buf);
                answer_buf[n] = (char)('0' + d);
                answer_buf[n + 1] = '\0';
                dirty = 1;
            } else if (k == GK_BACKSPACE && strlen(answer_buf) > 0) {
                answer_buf[strlen(answer_buf) - 1] = '\0';
                dirty = 1;
            } else if (k == GK_ENTER && strlen(answer_buf) > 0) {
                last_correct = (atoi(answer_buf) == a + b);
                feedback_start_ms = plat_time_ms();
                state = STATE_FEEDBACK;
                dirty = 1;
            }
        } else if (state == STATE_FEEDBACK) {
            if (plat_time_ms() - feedback_start_ms >= FEEDBACK_DURATION_MS) {
                generate_problem(&a, &b);
                answer_buf[0] = '\0';
                state = STATE_PROBLEM;
                dirty = 1;
            }
        }

        /* Redraw only when something actually changed: at 30fps a
         * single-buffered fbdev target flickers visibly if redrawn on a
         * fixed tick even though a turn-based game is static almost all
         * the time. */
        if (dirty) {
            vclear(COLOR_BG);

            if (state == STATE_PROBLEM) {
                char eq[16];
                snprintf(eq, sizeof(eq), "%d+%d=?", a, b);
                draw_centered(EQ_Y, EQ_HEIGHT, COLOR_TEXT, eq);
                draw_centered(ANS_Y, ANS_HEIGHT, COLOR_TEXT,
                              answer_buf[0] ? answer_buf : "_");
            } else { /* STATE_FEEDBACK */
                if (last_correct) {
                    vdraw_thick_line(380, 310, 460, 400, 26, COLOR_CORRECT);
                    vdraw_thick_line(460, 400, 640, 200, 26, COLOR_CORRECT);
                } else {
                    vdraw_thick_line(400, 200, 600, 400, 26, COLOR_WRONG);
                    vdraw_thick_line(600, 200, 400, 400, 26, COLOR_WRONG);
                }
            }

            plat_present();
            dirty = 0;
        }

        usleep(16000); /* ~60Hz input poll; drawing only happens on change */
    }

    ttf_shutdown();
    plat_shutdown();
    return 0;
}
