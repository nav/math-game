#include "progress.h"

#include <stdio.h>

int progress_load(const char *path, int *stage, int *subphase,
                   FactScheduler *sched) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int num_facts = 0;
    if (fscanf(f, "stage=%d\nsubphase=%d\nnum_facts=%d\n", stage, subphase,
               &num_facts) != 3 ||
        num_facts < 0 || num_facts > SCHED_MAX_FACTS) {
        fclose(f);
        return -1;
    }

    sched->num_facts = num_facts;
    for (int i = 0; i < num_facts; i++) {
        if (fscanf(f, "bucket=%d\n", &sched->bucket[i]) != 1) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

void progress_save(const char *path, int stage, int subphase,
                    const FactScheduler *sched) {
    FILE *f = fopen(path, "w");
    if (!f) return; /* best-effort: a lost save shouldn't crash the game */

    fprintf(f, "stage=%d\nsubphase=%d\nnum_facts=%d\n", stage, subphase,
            sched->num_facts);
    for (int i = 0; i < sched->num_facts; i++) {
        fprintf(f, "bucket=%d\n", sched->bucket[i]);
    }
    fclose(f);
}
