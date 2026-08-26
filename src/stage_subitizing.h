/* stage_subitizing.h - Stage 1 content: subitizing & counting.
 *
 * Five ordered steps, each gated by mastery of the previous one (see
 * curriculum.h): perceptual subitizing to 4, then to 5 (the age-4 vs.
 * age-5 developmental floor), then conceptual/composed subitizing 6-10
 * ("5 and 2 more"), then counting & cardinality to 5, then to 10. See
 * IMPLEMENTATION_PLAN.md Stage 1, sub-phases 0/(a)/(b)/(c).
 *
 * Facts accumulate across steps rather than being replaced: once a step
 * is reached, every fact introduced by it and all earlier steps stays in
 * the active drill pool (just at a lower drill frequency once mastered),
 * which is what gives interleaved/spaced practice across steps instead of
 * abandoning old facts the moment a new step unlocks.
 */
#ifndef STAGE_SUBITIZING_H
#define STAGE_SUBITIZING_H

typedef enum { SFK_PERCEPTUAL, SFK_COMPOSED, SFK_COUNT } SubitizeFactKind;

typedef struct {
    SubitizeFactKind kind;
    int quantity; /* 1-10 */
} SubitizeFact;

typedef enum {
    STEP_PERCEPTUAL_TO4,
    STEP_PERCEPTUAL_TO5,
    STEP_CONCEPTUAL_6_10,
    STEP_COUNTING_TO5,
    STEP_COUNTING_TO10,
    STEP_DONE, /* Stage 1 fully mastered; no Stage 2 content yet, so this
                * just keeps drilling the full pool. */
} SubitizeStep;

#define SUBITIZE_MAX_FACTS 20 /* 4 + 1 + 5 + 5 + 5 */

/* How many *new* facts step `step` itself introduces (not cumulative). */
int subitize_step_new_fact_count(SubitizeStep step);

/* Cumulative fact count across steps 0..step-1 - i.e. the pool index at
 * which `step`'s own new facts begin. */
int subitize_step_start_index(SubitizeStep step);

/* Fills out[] with every fact from step 0 up to and including upto_step
 * (or the full pool, for STEP_DONE). A fact already present at an earlier
 * step always lands at the same index on every call, so a fact_id from
 * the scheduler keeps meaning the same fact as the pool grows. Returns the
 * total fact count written. */
int subitize_build_pool(SubitizeStep upto_step, SubitizeFact out[SUBITIZE_MAX_FACTS]);

/* Short encouragement shown when advancing INTO this step. */
const char *subitize_step_intro_message(SubitizeStep step);

/* A fact's visual representation is randomized (jittered dot positions,
 * and for counting, which grid cells are used) so the same quantity never
 * looks pixel-identical twice in a row. That randomization must happen
 * exactly ONCE per problem, not on every redraw - the game loop redraws on
 * every keystroke (as the child types their answer), and recomputing
 * random positions on each of those redraws makes the dots visibly jump
 * around while answering, which is both distracting and, for the counting
 * task specifically, undermines the exercise. So: call
 * subitize_compute_layout() once when a new problem is chosen, cache the
 * result, and call subitize_draw_layout() on every redraw of that same
 * problem - it only reads the cached layout and never calls rand().
 *
 * Both depend on viewport.h; kept in a separate translation unit
 * (stage_subitizing_draw.c) from the pure pool logic above so host-only
 * unit tests can link the logic without pulling in a platform backend.
 */
#define SUBITIZE_MAX_DOTS 10

typedef struct {
    int x[SUBITIZE_MAX_DOTS];
    int y[SUBITIZE_MAX_DOTS];
    int count;
    const char *prompt; /* "How many?" or "How many in total?" */
} SubitizeLayout;

void subitize_compute_layout(const SubitizeFact *fact, SubitizeLayout *out);
void subitize_draw_layout(const SubitizeLayout *layout);

#endif
