/* test_curriculum.c - host-only unit tests for curriculum.c / progress.c.
 * No platform/viewport/font dependency; run via `make test` in src/.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/curriculum.h"
#include "../src/progress.h"
#include "../src/stage_subitizing.h"

static void test_mastery_not_met_until_window_full(void) {
    MasteryTracker m;
    mastery_init(&m);
    for (int i = 0; i < MASTERY_WINDOW - 1; i++) {
        mastery_record(&m, 1);
        assert(!mastery_met(&m));
    }
}

static void test_mastery_met_at_threshold(void) {
    MasteryTracker m;
    mastery_init(&m);
    /* Exactly MASTERY_THRESHOLD correct, rest wrong. */
    for (int i = 0; i < MASTERY_WINDOW; i++) {
        mastery_record(&m, i < MASTERY_THRESHOLD);
    }
    assert(mastery_met(&m));
}

static void test_mastery_not_met_below_threshold(void) {
    MasteryTracker m;
    mastery_init(&m);
    for (int i = 0; i < MASTERY_WINDOW; i++) {
        mastery_record(&m, i < MASTERY_THRESHOLD - 1);
    }
    assert(!mastery_met(&m));
}

static void test_mastery_window_slides(void) {
    /* Old attempts age out: MASTERY_WINDOW correct answers, then enough
     * wrong ones to slide every correct answer out of the window. */
    MasteryTracker m;
    mastery_init(&m);
    for (int i = 0; i < MASTERY_WINDOW; i++) mastery_record(&m, 1);
    assert(mastery_met(&m));
    for (int i = 0; i < MASTERY_WINDOW; i++) mastery_record(&m, 0);
    assert(!mastery_met(&m));
}

static void test_sched_record_caps_and_resets_bucket(void) {
    FactScheduler s;
    sched_init(&s, 4);
    for (int i = 0; i < 10; i++) sched_record(&s, 0, 1);
    assert(s.bucket[0] == SCHED_NUM_BUCKETS - 1);
    sched_record(&s, 0, 0);
    assert(s.bucket[0] == 0);
}

static void test_sched_pick_favors_low_buckets(void) {
    FactScheduler s;
    sched_init(&s, 2);
    /* Fact 0 stays at bucket 0 (just missed); fact 1 gets promoted to the
     * top bucket (mastered). Fact 0 should be picked noticeably more
     * often, but fact 1 must still be reachable (spaced retrieval never
     * fully abandons mastered facts). */
    for (int i = 0; i < SCHED_NUM_BUCKETS + 5; i++) sched_record(&s, 1, 1);
    assert(s.bucket[1] == SCHED_NUM_BUCKETS - 1);
    assert(s.bucket[0] == 0);

    srand(42);
    int picks[2] = {0, 0};
    const int trials = 4000;
    for (int i = 0; i < trials; i++) picks[sched_pick(&s)]++;

    assert(picks[0] > picks[1]);
    assert(picks[1] > 0); /* still resurfaces occasionally */
}

static void test_progress_round_trip(void) {
    FactScheduler s;
    sched_init(&s, 5);
    sched_record(&s, 2, 1);
    sched_record(&s, 2, 1);
    sched_record(&s, 4, 0);

    const char *path = "/tmp/math_game_test_progress.dat";
    progress_save(path, 1, 3, &s);

    int stage = -1, subphase = -1;
    FactScheduler loaded;
    int rc = progress_load(path, &stage, &subphase, &loaded);
    assert(rc == 0);
    assert(stage == 1);
    assert(subphase == 3);
    assert(loaded.num_facts == 5);
    for (int i = 0; i < 5; i++) assert(loaded.bucket[i] == s.bucket[i]);

    remove(path);
}

static void test_progress_load_missing_file(void) {
    int stage, subphase;
    FactScheduler s;
    int rc = progress_load("/tmp/math_game_test_missing_XYZ.dat", &stage,
                            &subphase, &s);
    assert(rc == -1);
}

static void test_sched_grow_preserves_existing_buckets(void) {
    FactScheduler s;
    sched_init(&s, 2);
    sched_record(&s, 0, 1);
    sched_record(&s, 0, 1); /* bucket[0] now 2 */
    sched_grow(&s, 5);
    assert(s.num_facts == 5);
    assert(s.bucket[0] == 2); /* untouched */
    assert(s.bucket[1] == 0);
    assert(s.bucket[2] == 0);
    assert(s.bucket[4] == 0);
}

static void test_subitize_step_fact_counts_and_starts(void) {
    assert(subitize_step_new_fact_count(STEP_PERCEPTUAL_TO4) == 4);
    assert(subitize_step_new_fact_count(STEP_PERCEPTUAL_TO5) == 1);
    assert(subitize_step_new_fact_count(STEP_CONCEPTUAL_6_10) == 5);
    assert(subitize_step_new_fact_count(STEP_COUNTING_TO5) == 5);
    assert(subitize_step_new_fact_count(STEP_COUNTING_TO10) == 5);
    assert(subitize_step_new_fact_count(STEP_DONE) == 0);

    assert(subitize_step_start_index(STEP_PERCEPTUAL_TO4) == 0);
    assert(subitize_step_start_index(STEP_PERCEPTUAL_TO5) == 4);
    assert(subitize_step_start_index(STEP_CONCEPTUAL_6_10) == 5);
    assert(subitize_step_start_index(STEP_COUNTING_TO5) == 10);
    assert(subitize_step_start_index(STEP_COUNTING_TO10) == 15);
    assert(subitize_step_start_index(STEP_DONE) == 20);
}

static void test_subitize_pool_is_cumulative_and_stable(void) {
    SubitizeFact pool4[SUBITIZE_MAX_FACTS];
    SubitizeFact pool5[SUBITIZE_MAX_FACTS];
    int n4 = subitize_build_pool(STEP_PERCEPTUAL_TO4, pool4);
    int n5 = subitize_build_pool(STEP_PERCEPTUAL_TO5, pool5);
    assert(n4 == 4);
    assert(n5 == 5);
    /* Every fact from the smaller pool must appear at the same index in
     * the larger one - this is what makes a scheduler fact_id keep
     * meaning the same fact as the pool grows across steps. */
    for (int i = 0; i < n4; i++) {
        assert(pool4[i].kind == pool5[i].kind);
        assert(pool4[i].quantity == pool5[i].quantity);
    }
    assert(pool5[4].kind == SFK_PERCEPTUAL);
    assert(pool5[4].quantity == 5);

    SubitizeFact pool_done[SUBITIZE_MAX_FACTS];
    int n_done = subitize_build_pool(STEP_DONE, pool_done);
    assert(n_done == SUBITIZE_MAX_FACTS);
    /* Composed facts (6-10) and count facts (1-10) coexist as distinct
     * facts even where their quantities overlap. */
    int composed_seen = 0, count_seen = 0;
    for (int i = 0; i < n_done; i++) {
        if (pool_done[i].kind == SFK_COMPOSED && pool_done[i].quantity == 7)
            composed_seen++;
        if (pool_done[i].kind == SFK_COUNT && pool_done[i].quantity == 7)
            count_seen++;
    }
    assert(composed_seen == 1);
    assert(count_seen == 1);
}

int main(void) {
    test_mastery_not_met_until_window_full();
    test_mastery_met_at_threshold();
    test_mastery_not_met_below_threshold();
    test_mastery_window_slides();
    test_sched_record_caps_and_resets_bucket();
    test_sched_pick_favors_low_buckets();
    test_progress_round_trip();
    test_progress_load_missing_file();
    test_sched_grow_preserves_existing_buckets();
    test_subitize_step_fact_counts_and_starts();
    test_subitize_pool_is_cumulative_and_stable();
    printf("all tests passed\n");
    return 0;
}
