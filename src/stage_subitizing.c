/* stage_subitizing.c - pure step/pool logic, no rendering dependency.
 * See stage_subitizing_draw.c for the visual side.
 */
#include "stage_subitizing.h"

#include <stddef.h>

static const int kStepNewFactCount[STEP_DONE] = {4, 1, 5, 5, 5};

int subitize_step_new_fact_count(SubitizeStep step) {
    if (step < 0 || step >= STEP_DONE) return 0;
    return kStepNewFactCount[step];
}

int subitize_step_start_index(SubitizeStep step) {
    int idx = 0;
    int last = step < STEP_DONE ? (int)step : (int)STEP_DONE;
    for (int s = 0; s < last; s++) idx += kStepNewFactCount[s];
    return idx;
}

int subitize_build_pool(SubitizeStep upto_step,
                         SubitizeFact out[SUBITIZE_MAX_FACTS]) {
    int idx = 0;
    SubitizeStep last = upto_step == STEP_DONE ? STEP_COUNTING_TO10 : upto_step;

    for (SubitizeStep s = STEP_PERCEPTUAL_TO4; s <= last; s++) {
        switch (s) {
            case STEP_PERCEPTUAL_TO4:
                for (int q = 1; q <= 4; q++)
                    out[idx++] = (SubitizeFact){SFK_PERCEPTUAL, q};
                break;
            case STEP_PERCEPTUAL_TO5:
                out[idx++] = (SubitizeFact){SFK_PERCEPTUAL, 5};
                break;
            case STEP_CONCEPTUAL_6_10:
                for (int q = 6; q <= 10; q++)
                    out[idx++] = (SubitizeFact){SFK_COMPOSED, q};
                break;
            case STEP_COUNTING_TO5:
                for (int q = 1; q <= 5; q++)
                    out[idx++] = (SubitizeFact){SFK_COUNT, q};
                break;
            case STEP_COUNTING_TO10:
                for (int q = 6; q <= 10; q++)
                    out[idx++] = (SubitizeFact){SFK_COUNT, q};
                break;
            case STEP_DONE:
                break;
        }
    }
    return idx;
}

const char *subitize_step_intro_message(SubitizeStep step) {
    switch (step) {
        case STEP_PERCEPTUAL_TO5:
            return "Great counting! Let's try up to 5!";
        case STEP_CONCEPTUAL_6_10:
            return "Nice! Now let's go past 5!";
        case STEP_COUNTING_TO5:
            return "Let's count them all!";
        case STEP_COUNTING_TO10:
            return "Let's count all the way to 10!";
        case STEP_DONE:
            return "Amazing! You've mastered counting!";
        default:
            return "Great job!";
    }
}
