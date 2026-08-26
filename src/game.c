/* game.c - shared game logic and main loop.
 *
 * Platform-independent: only talks to the plat_ and v-prefixed functions
 * declared in platform.h / viewport.h, so the exact same file builds
 * against either
 * platform_fbdev.c (Pi) or platform_sdl.c (macOS dev loop).
 *
 * Stage 1 (see IMPLEMENTATION_PLAN.md): subitizing & counting, five
 * mastery-gated steps with a cumulative, interleaved drill pool (see
 * stage_subitizing.h). Answer input is keyboard digit + Enter for now; the
 * physical NFC/button hardware (Stage H1-H2) plugs into the same "answer
 * value + confirm" event this loop already drives from, once built.
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "curriculum.h"
#include "platform.h"
#include "progress.h"
#include "stage_subitizing.h"
#include "ttf_font.h"
#include "viewport.h"

#ifndef FONT_PATH
#define FONT_PATH "../assets/DejaVuSans-Bold.ttf"
#endif

static const Color COLOR_BG = {20, 20, 45};
static const Color COLOR_TEXT = {235, 235, 240};
static const Color COLOR_CORRECT = {40, 200, 90};
static const Color COLOR_WRONG = {220, 60, 60};

#define CORRECT_FEEDBACK_MS 1200
#define WRONG_FEEDBACK_MS 2400 /* longer: there's corrective text to read */
#define MASTERY_MSG_MS 2200

#define PROMPT_Y 380
#define PROMPT_HEIGHT 70
#define ANS_Y 460
#define ANS_HEIGHT 90
#define CORRECTIVE_Y 420
#define CORRECTIVE_HEIGHT 50

/* Curriculum stage identifier persisted to disk. Only one exists so far;
 * the field exists now so progress.dat's format doesn't need to change
 * when Stage 2 is added. */
#define STAGE_SUBITIZING 1

typedef enum { STATE_PROBLEM, STATE_FEEDBACK, STATE_MASTERY } GameState;

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

    SubitizeStep step = STEP_PERCEPTUAL_TO4;
    SubitizeFact pool[SUBITIZE_MAX_FACTS];
    FactScheduler sched;
    MasteryTracker mastery;

    int loaded_stage, loaded_subphase;
    int pool_len;
    if (progress_load(PROGRESS_PATH, &loaded_stage, &loaded_subphase,
                       &sched) == 0 &&
        loaded_stage == STAGE_SUBITIZING && loaded_subphase >= 0 &&
        loaded_subphase <= STEP_DONE) {
        step = (SubitizeStep)loaded_subphase;
        pool_len = subitize_build_pool(step, pool);
        if (pool_len != sched.num_facts) {
            /* Corrupted/stale save: pool shape doesn't match. Start over. */
            step = STEP_PERCEPTUAL_TO4;
            pool_len = subitize_build_pool(step, pool);
            sched_init(&sched, pool_len);
        }
    } else {
        pool_len = subitize_build_pool(step, pool);
        sched_init(&sched, pool_len);
    }
    mastery_init(&mastery);

    GameState state = STATE_PROBLEM;
    char answer_buf[3] = {0};
    int fact_id = sched_pick(&sched);
    int quantity = pool[fact_id].quantity;
    SubitizeLayout layout;
    subitize_compute_layout(&pool[fact_id], &layout);
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
                last_correct = (atoi(answer_buf) == quantity);
                sched_record(&sched, fact_id, last_correct);
                /* Only the current step's own new facts count toward
                 * advancing it - earlier facts are drilled for retention
                 * (spaced/interleaved), not re-assessed as "the skill". */
                if (fact_id >= subitize_step_start_index(step)) {
                    mastery_record(&mastery, last_correct);
                }
                progress_save(PROGRESS_PATH, STAGE_SUBITIZING, step, &sched);
                feedback_start_ms = plat_time_ms();
                state = STATE_FEEDBACK;
                dirty = 1;
            }
        } else if (state == STATE_FEEDBACK) {
            long elapsed = plat_time_ms() - feedback_start_ms;
            long duration =
                last_correct ? CORRECT_FEEDBACK_MS : WRONG_FEEDBACK_MS;
            if (elapsed >= duration) {
                if (step != STEP_DONE && mastery_met(&mastery)) {
                    step = (SubitizeStep)(step + 1);
                    pool_len = subitize_build_pool(step, pool);
                    sched_grow(&sched, pool_len);
                    mastery_init(&mastery);
                    progress_save(PROGRESS_PATH, STAGE_SUBITIZING, step,
                                  &sched);
                    feedback_start_ms = plat_time_ms();
                    state = STATE_MASTERY;
                } else {
                    fact_id = sched_pick(&sched);
                    quantity = pool[fact_id].quantity;
                    subitize_compute_layout(&pool[fact_id], &layout);
                    answer_buf[0] = '\0';
                    state = STATE_PROBLEM;
                }
                dirty = 1;
            }
        } else if (state == STATE_MASTERY) {
            if (plat_time_ms() - feedback_start_ms >= MASTERY_MSG_MS) {
                fact_id = sched_pick(&sched);
                quantity = pool[fact_id].quantity;
                subitize_compute_layout(&pool[fact_id], &layout);
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
                subitize_draw_layout(&layout);
                draw_centered(PROMPT_Y, PROMPT_HEIGHT, COLOR_TEXT,
                              layout.prompt);
                draw_centered(ANS_Y, ANS_HEIGHT, COLOR_TEXT,
                              answer_buf[0] ? answer_buf : "_");
            } else if (state == STATE_FEEDBACK) {
                if (last_correct) {
                    vdraw_thick_line(380, 310, 460, 400, 26, COLOR_CORRECT);
                    vdraw_thick_line(460, 400, 640, 200, 26, COLOR_CORRECT);
                } else {
                    vdraw_thick_line(400, 200, 600, 400, 26, COLOR_WRONG);
                    vdraw_thick_line(600, 200, 400, 400, 26, COLOR_WRONG);
                    char corrective[24];
                    snprintf(corrective, sizeof(corrective), "It was %d",
                             quantity);
                    draw_centered(CORRECTIVE_Y, CORRECTIVE_HEIGHT, COLOR_TEXT,
                                  corrective);
                }
            } else { /* STATE_MASTERY */
                draw_centered(300, 70, COLOR_CORRECT,
                              subitize_step_intro_message(step));
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
