/* game.c - shared game logic and main loop.
 *
 * Platform-independent: only talks to the plat_ and v-prefixed functions
 * declared in platform.h / viewport.h, so the exact same file builds
 * against either
 * platform_fbdev.c (Pi) or platform_sdl.c (macOS dev loop).
 *
 * Two curriculum stages exist (see IMPLEMENTATION_PLAN.md): Stage 1
 * subitizing & counting, Stage 2 number bonds & addition strategies. Both
 * use the same mastery-gated, cumulative/interleaved drill pool pattern
 * (curriculum.h) but generate/draw completely different content
 * (stage_subitizing.h vs stage_addition.h), so this file dispatches on
 * `curriculum_stage` at the handful of points where that content actually
 * differs, rather than through a generic plugin abstraction - with exactly
 * two stages an if/else is the boring, obvious choice; a real dispatch
 * table would earn its keep once a third stage makes the branching
 * unwieldy, not before.
 *
 * Answer input is keyboard digit + Enter for now; the physical NFC/button
 * hardware (Stage H1-H2) plugs into the same "answer value + confirm"
 * event this loop already drives from, once built.
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
#include "stage_addition.h"
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
#define MASTERY_MSG_MS 3200    /* longer: it's a rare, worth-savoring moment */

#define PROMPT_Y 380
#define PROMPT_HEIGHT 70
#define ANS_Y 460
#define ANS_HEIGHT 90
#define CORRECTIVE_Y 420
#define CORRECTIVE_HEIGHT 50

/* Addition's equation gets its own (larger) font size and layout, separate
 * from Stage 1's PROMPT_HEIGHT/ANS_Y - equations are short and bounded
 * ("9 + 9 = ?" etc.), so a fixed larger size is safe (verified against the
 * actual font metrics; the longest equation stays well under half the
 * canvas width even at this size), unlike the earlier mastery-message bug
 * where message length varied too much for any single fixed size to be
 * safe. ADD_EQ_ANS_GAP is deliberately generous relative to ADD_EQ_HEIGHT
 * so the taller glyphs can't run down into the answer line below them. */
#define ADD_EQ_HEIGHT 100
#define ADD_EQ_ANS_GAP 120

/* Below a diagram (counting-on/make-ten): diagrams end by y~330, so 360
 * leaves clear margin above; the answer at 360+120=480 leaves 30px below
 * it before the canvas edge (480+ANS_HEIGHT=570 of 600). */
#define ADD_EQ_Y_DIAGRAM 360
#define ADD_ANS_Y_DIAGRAM (ADD_EQ_Y_DIAGRAM + ADD_EQ_ANS_GAP)

/* No diagram (small sums/bonds/doubles): center the equation+answer block
 * (height ADD_EQ_ANS_GAP + ANS_HEIGHT) in the play area, same idea as
 * Stage 1's dots-then-prompt-then-answer flow but without dots to anchor
 * against. */
#define ADD_EQ_Y_NO_DIAGRAM ((VIRTUAL_H - (ADD_EQ_ANS_GAP + ANS_HEIGHT)) / 2)
#define ADD_ANS_Y_NO_DIAGRAM (ADD_EQ_Y_NO_DIAGRAM + ADD_EQ_ANS_GAP)

#define MASTERY_MSG_Y 300
#define MASTERY_MSG_MAX_HEIGHT 70
#define MASTERY_MSG_MAX_WIDTH 900 /* leaves a margin on a 1000-wide canvas */

/* Curriculum stage identifiers persisted to disk (the `stage` field of
 * progress.dat; `subphase` holds the stage's own step enum value). */
#define STAGE_SUBITIZING 1
#define STAGE_ADDITION 2

typedef enum { STATE_PROBLEM, STATE_FEEDBACK, STATE_MASTERY } GameState;

static int key_to_digit(Key k) {
    if (k >= GK_0 && k <= GK_9) return k - GK_0;
    return -1;
}

static void draw_centered(int cy, int v_height, Color c, const char *text) {
    int w = ttf_text_width(v_height, text);
    ttf_draw_text((VIRTUAL_W - w) / 2, cy, v_height, c, text);
}

/* Like draw_centered, but shrinks the font size to keep the text within
 * max_width instead of letting it run off both edges of the canvas - used
 * for the mastery/step-transition messages, whose length varies a lot
 * from stage to stage ("Great job!" vs. "You're a counting star! Let's
 * learn addition!") and isn't something callers should have to hand-tune
 * a safe font size for individually. */
static void draw_centered_fit(int cy, int max_height, int max_width, Color c,
                               const char *text) {
    int v_height = max_height;
    int w = ttf_text_width(v_height, text);
    if (w > max_width) {
        v_height = v_height * max_width / w;
        if (v_height < 20) v_height = 20; /* floor: stay legible */
    }
    draw_centered(cy, v_height, c, text);
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

    int curriculum_stage = STAGE_SUBITIZING;
    SubitizeStep sub_step = STEP_PERCEPTUAL_TO4;
    SubitizeFact sub_pool[SUBITIZE_MAX_FACTS];
    SubitizeLayout sub_layout;
    AdditionStep add_step = ASTEP_SMALL_SUMS;
    AdditionFact add_pool[ADDITION_MAX_FACTS];
    FactScheduler sched;
    MasteryTracker mastery;

    int loaded_stage, loaded_subphase, pool_len;
    int loaded_ok = progress_load(PROGRESS_PATH, &loaded_stage,
                                   &loaded_subphase, &sched) == 0;

    if (loaded_ok && loaded_stage == STAGE_SUBITIZING &&
        loaded_subphase >= 0 && loaded_subphase <= STEP_DONE) {
        curriculum_stage = STAGE_SUBITIZING;
        sub_step = (SubitizeStep)loaded_subphase;
        pool_len = subitize_build_pool(sub_step, sub_pool);
        if (pool_len != sched.num_facts) { /* corrupted/stale save */
            sub_step = STEP_PERCEPTUAL_TO4;
            pool_len = subitize_build_pool(sub_step, sub_pool);
            sched_init(&sched, pool_len);
        }
    } else if (loaded_ok && loaded_stage == STAGE_ADDITION &&
               loaded_subphase >= 0 && loaded_subphase <= ASTEP_DONE) {
        curriculum_stage = STAGE_ADDITION;
        add_step = (AdditionStep)loaded_subphase;
        pool_len = addition_build_pool(add_step, add_pool);
        if (pool_len != sched.num_facts) { /* corrupted/stale save */
            add_step = ASTEP_SMALL_SUMS;
            pool_len = addition_build_pool(add_step, add_pool);
            sched_init(&sched, pool_len);
        }
    } else {
        curriculum_stage = STAGE_SUBITIZING;
        sub_step = STEP_PERCEPTUAL_TO4;
        pool_len = subitize_build_pool(sub_step, sub_pool);
        sched_init(&sched, pool_len);
    }
    mastery_init(&mastery);

    GameState state = STATE_PROBLEM;
    char answer_buf[3] = {0};
    int fact_id = sched_pick(&sched);
    int correct_answer;
    if (curriculum_stage == STAGE_SUBITIZING) {
        correct_answer = sub_pool[fact_id].quantity;
        subitize_compute_layout(&sub_pool[fact_id], &sub_layout);
    } else {
        correct_answer = add_pool[fact_id].answer;
    }
    const char *mastery_msg = "";
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
                last_correct = (atoi(answer_buf) == correct_answer);
                sched_record(&sched, fact_id, last_correct);
                /* Only the current step's own new facts count toward
                 * advancing it - earlier facts are drilled for retention
                 * (spaced/interleaved), not re-assessed as "the skill". */
                int step_start = curriculum_stage == STAGE_SUBITIZING
                                      ? subitize_step_start_index(sub_step)
                                      : addition_step_start_index(add_step);
                if (fact_id >= step_start) mastery_record(&mastery, last_correct);
                progress_save(PROGRESS_PATH, curriculum_stage,
                              curriculum_stage == STAGE_SUBITIZING
                                  ? (int)sub_step
                                  : (int)add_step,
                              &sched);
                feedback_start_ms = plat_time_ms();
                state = STATE_FEEDBACK;
                dirty = 1;
            }
        } else if (state == STATE_FEEDBACK) {
            long elapsed = plat_time_ms() - feedback_start_ms;
            long duration =
                last_correct ? CORRECT_FEEDBACK_MS : WRONG_FEEDBACK_MS;
            if (elapsed >= duration) {
                int advanced = 0;
                if (curriculum_stage == STAGE_SUBITIZING &&
                    sub_step != STEP_DONE && mastery_met(&mastery)) {
                    sub_step = (SubitizeStep)(sub_step + 1);
                    if (sub_step == STEP_DONE) {
                        /* Stage 1 fully mastered: move straight into
                         * Stage 2 rather than drilling STEP_DONE forever
                         * (that resting state only exists because Stage 2
                         * didn't, until now). */
                        curriculum_stage = STAGE_ADDITION;
                        add_step = ASTEP_SMALL_SUMS;
                        pool_len = addition_build_pool(add_step, add_pool);
                        sched_init(&sched, pool_len);
                        mastery_msg = "You're a counting star! Let's learn addition!";
                    } else {
                        pool_len = subitize_build_pool(sub_step, sub_pool);
                        sched_grow(&sched, pool_len);
                        mastery_msg = subitize_step_intro_message(sub_step);
                    }
                    mastery_init(&mastery);
                    progress_save(PROGRESS_PATH, curriculum_stage,
                                  curriculum_stage == STAGE_SUBITIZING
                                      ? (int)sub_step
                                      : (int)add_step,
                                  &sched);
                    feedback_start_ms = plat_time_ms();
                    state = STATE_MASTERY;
                    advanced = 1;
                } else if (curriculum_stage == STAGE_ADDITION &&
                           add_step != ASTEP_DONE && mastery_met(&mastery)) {
                    add_step = (AdditionStep)(add_step + 1);
                    pool_len = addition_build_pool(add_step, add_pool);
                    sched_grow(&sched, pool_len);
                    mastery_init(&mastery);
                    mastery_msg = addition_step_intro_message(add_step);
                    progress_save(PROGRESS_PATH, curriculum_stage, add_step,
                                  &sched);
                    feedback_start_ms = plat_time_ms();
                    state = STATE_MASTERY;
                    advanced = 1;
                }

                if (!advanced) {
                    fact_id = sched_pick(&sched);
                    if (curriculum_stage == STAGE_SUBITIZING) {
                        correct_answer = sub_pool[fact_id].quantity;
                        subitize_compute_layout(&sub_pool[fact_id], &sub_layout);
                    } else {
                        correct_answer = add_pool[fact_id].answer;
                    }
                    answer_buf[0] = '\0';
                    state = STATE_PROBLEM;
                }
                dirty = 1;
            }
        } else if (state == STATE_MASTERY) {
            if (plat_time_ms() - feedback_start_ms >= MASTERY_MSG_MS) {
                fact_id = sched_pick(&sched);
                if (curriculum_stage == STAGE_SUBITIZING) {
                    correct_answer = sub_pool[fact_id].quantity;
                    subitize_compute_layout(&sub_pool[fact_id], &sub_layout);
                } else {
                    correct_answer = add_pool[fact_id].answer;
                }
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
                int prompt_y = PROMPT_Y;
                int ans_y = ANS_Y;
                if (curriculum_stage == STAGE_SUBITIZING) {
                    subitize_draw_layout(&sub_layout);
                    draw_centered(prompt_y, PROMPT_HEIGHT, COLOR_TEXT,
                                  sub_layout.prompt);
                } else {
                    if (addition_fact_has_diagram(add_pool[fact_id].kind)) {
                        prompt_y = ADD_EQ_Y_DIAGRAM;
                        ans_y = ADD_ANS_Y_DIAGRAM;
                    } else {
                        prompt_y = ADD_EQ_Y_NO_DIAGRAM;
                        ans_y = ADD_ANS_Y_NO_DIAGRAM;
                    }
                    char eq[32];
                    addition_draw_fact(&add_pool[fact_id], eq, sizeof(eq));
                    draw_centered(prompt_y, ADD_EQ_HEIGHT, COLOR_TEXT, eq);
                }
                draw_centered(ans_y, ANS_HEIGHT, COLOR_TEXT,
                              answer_buf[0] ? answer_buf : "_");
            } else if (state == STATE_FEEDBACK) {
                if (last_correct) {
                    vdraw_thick_line(380, 310, 460, 400, 26, COLOR_CORRECT);
                    vdraw_thick_line(460, 400, 640, 200, 26, COLOR_CORRECT);
                } else {
                    vdraw_thick_line(400, 200, 600, 400, 26, COLOR_WRONG);
                    vdraw_thick_line(600, 200, 400, 400, 26, COLOR_WRONG);
                    char corrective[32];
                    if (curriculum_stage == STAGE_SUBITIZING) {
                        snprintf(corrective, sizeof(corrective), "It was %d",
                                 correct_answer);
                    } else {
                        const AdditionFact *f = &add_pool[fact_id];
                        if (f->kind == ADD_BONDS_TO_10) {
                            snprintf(corrective, sizeof(corrective),
                                     "%d + %d = 10", f->a, f->b);
                        } else {
                            snprintf(corrective, sizeof(corrective),
                                     "%d + %d = %d", f->a, f->b, f->answer);
                        }
                    }
                    draw_centered(CORRECTIVE_Y, CORRECTIVE_HEIGHT, COLOR_TEXT,
                                  corrective);
                }
            } else { /* STATE_MASTERY */
                draw_centered_fit(MASTERY_MSG_Y, MASTERY_MSG_MAX_HEIGHT,
                                   MASTERY_MSG_MAX_WIDTH, COLOR_CORRECT,
                                   mastery_msg);
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
