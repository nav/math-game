#include "curriculum.h"

#include <stdlib.h>

void mastery_init(MasteryTracker *m) {
    m->filled = 0;
    m->next = 0;
    for (int i = 0; i < MASTERY_WINDOW; i++) m->history[i] = 0;
}

void mastery_record(MasteryTracker *m, int correct) {
    m->history[m->next] = correct ? 1 : 0;
    m->next = (m->next + 1) % MASTERY_WINDOW;
    if (m->filled < MASTERY_WINDOW) m->filled++;
}

int mastery_met(const MasteryTracker *m) {
    if (m->filled < MASTERY_WINDOW) return 0;
    int sum = 0;
    for (int i = 0; i < MASTERY_WINDOW; i++) sum += m->history[i];
    return sum >= MASTERY_THRESHOLD;
}

void sched_init(FactScheduler *s, int num_facts) {
    s->num_facts = num_facts;
    for (int i = 0; i < num_facts; i++) s->bucket[i] = 0;
}

void sched_grow(FactScheduler *s, int new_num_facts) {
    for (int i = s->num_facts; i < new_num_facts; i++) s->bucket[i] = 0;
    s->num_facts = new_num_facts;
}

int sched_pick(const FactScheduler *s) {
    int total = 0;
    for (int i = 0; i < s->num_facts; i++) {
        total += SCHED_NUM_BUCKETS - s->bucket[i];
    }
    int r = rand() % total;
    for (int i = 0; i < s->num_facts; i++) {
        int weight = SCHED_NUM_BUCKETS - s->bucket[i];
        if (r < weight) return i;
        r -= weight;
    }
    return s->num_facts - 1; /* unreachable */
}

void sched_record(FactScheduler *s, int fact_id, int correct) {
    if (correct) {
        if (s->bucket[fact_id] < SCHED_NUM_BUCKETS - 1) s->bucket[fact_id]++;
    } else {
        s->bucket[fact_id] = 0;
    }
}
