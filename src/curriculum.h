/* curriculum.h - stage-agnostic mastery tracking and item scheduling.
 *
 * Any content stage (subitizing, addition, ...) drills a small set of
 * "facts" (e.g. one fact per quantity, or one per operand pair). This
 * module tracks whether the *current phase* has been mastered (rolling-
 * window accuracy) and which fact to drill next (a Leitner-style scheduler
 * that resurfaces recently-missed facts sooner than mastered ones), without
 * knowing anything about what a fact actually is. See IMPLEMENTATION_PLAN.md
 * Stage 0.
 */
#ifndef CURRICULUM_H
#define CURRICULUM_H

#define MASTERY_WINDOW 8
#define MASTERY_THRESHOLD 6 /* out of MASTERY_WINDOW, to advance */

typedef struct {
    int history[MASTERY_WINDOW]; /* 1 = correct, 0 = incorrect */
    int filled;                  /* how many slots hold real data, caps at WINDOW */
    int next;                    /* circular write position */
} MasteryTracker;

void mastery_init(MasteryTracker *m);
void mastery_record(MasteryTracker *m, int correct);
/* True once the window is full and at least MASTERY_THRESHOLD are correct. */
int mastery_met(const MasteryTracker *m);

#define SCHED_MAX_FACTS 80 /* Stage 1 uses 20, Stage 2 uses 63; headroom for later stages */
#define SCHED_NUM_BUCKETS 4 /* 0 = just missed (resurfaces soonest) */

typedef struct {
    int bucket[SCHED_MAX_FACTS];
    int num_facts;
} FactScheduler;

void sched_init(FactScheduler *s, int num_facts);
/* Extends the pool to new_num_facts, initializing only the newly-added
 * slots to bucket 0; existing facts' buckets are left untouched. Used when
 * a curriculum step unlocks and adds facts to an already-in-progress pool
 * without resetting progress on what's already there. */
void sched_grow(FactScheduler *s, int new_num_facts);
/* Weighted-random pick favoring low buckets; never fully abandons
 * high-bucket (mastered) facts, matching the spaced-retrieval requirement
 * that mastered facts keep resurfacing, just less often. */
int sched_pick(const FactScheduler *s);
void sched_record(FactScheduler *s, int fact_id, int correct);

#endif
