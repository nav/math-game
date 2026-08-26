/* stage_addition.c - pure step/pool logic, no rendering dependency.
 * See stage_addition_draw.c for the visual side.
 */
#include "stage_addition.h"

static const int kStepNewFactCount[ASTEP_DONE] = {25, 9, 12, 9, 8};

int addition_step_new_fact_count(AdditionStep step) {
    if (step < 0 || step >= ASTEP_DONE) return 0;
    return kStepNewFactCount[step];
}

int addition_step_start_index(AdditionStep step) {
    int idx = 0;
    int last = step < ASTEP_DONE ? (int)step : (int)ASTEP_DONE;
    for (int s = 0; s < last; s++) idx += kStepNewFactCount[s];
    return idx;
}

int addition_build_pool(AdditionStep upto_step,
                         AdditionFact out[ADDITION_MAX_FACTS]) {
    int idx = 0;
    AdditionStep last = upto_step == ASTEP_DONE ? ASTEP_MAKE_TEN : upto_step;

    for (AdditionStep s = ASTEP_SMALL_SUMS; s <= last; s++) {
        switch (s) {
            case ASTEP_SMALL_SUMS:
                /* Plain a+b, no strategy framing, both addends small
                 * enough to count on fingers if needed - the easy on-ramp
                 * before an unknown-addend problem like bonds-to-10. */
                for (int a = 1; a <= 5; a++) {
                    for (int b = 1; b <= 5; b++) {
                        out[idx++] =
                            (AdditionFact){ADD_SMALL_SUM, a, b, a + b};
                    }
                }
                break;
            case ASTEP_BONDS_TO_10:
                /* "a and __ make 10" - the classic unknown-addend framing,
                 * a in 1..9 so every bond pair (including 5+5) appears. */
                for (int a = 1; a <= 9; a++) {
                    int b = 10 - a;
                    out[idx++] = (AdditionFact){ADD_BONDS_TO_10, a, b, b};
                }
                break;
            case ASTEP_COUNTING_ON:
                /* Counting-on only pays off when the smaller addend is
                 * small enough to count in your head (1-3) and the larger
                 * is big enough that direct modeling would be tedious
                 * (6-9). */
                for (int larger = 6; larger <= 9; larger++) {
                    for (int smaller = 1; smaller <= 3; smaller++) {
                        out[idx++] = (AdditionFact){
                            ADD_COUNTING_ON, larger, smaller, larger + smaller};
                    }
                }
                break;
            case ASTEP_DOUBLES:
                for (int n = 1; n <= 9; n++) {
                    out[idx++] = (AdditionFact){ADD_DOUBLES, n, n, 2 * n};
                }
                break;
            case ASTEP_MAKE_TEN:
                /* a in 6..9 so completing the ten-frame (10-a) always
                 * leaves a nonzero, single-digit gap; b must exceed that
                 * gap or there'd be nothing left to bridge with. Two b
                 * values per a: the smallest possible bridge, and the
                 * largest (9), for some spread. */
                for (int a = 6; a <= 9; a++) {
                    int gap = 10 - a;
                    int bs[2] = {gap + 1, 9};
                    for (int i = 0; i < 2; i++) {
                        int b = bs[i];
                        out[idx++] =
                            (AdditionFact){ADD_MAKE_TEN, a, b, a + b};
                    }
                }
                break;
            case ASTEP_DONE:
                break;
        }
    }
    return idx;
}

int addition_fact_has_diagram(AdditionFactKind kind) {
    return kind == ADD_COUNTING_ON || kind == ADD_MAKE_TEN;
}

const char *addition_step_intro_message(AdditionStep step) {
    switch (step) {
        case ASTEP_BONDS_TO_10:
            return "Let's find friends of 10!";
        case ASTEP_COUNTING_ON:
            return "Let's count on to add!";
        case ASTEP_DOUBLES:
            return "Now let's try doubles!";
        case ASTEP_MAKE_TEN:
            return "Let's make ten to add bigger numbers!";
        case ASTEP_DONE:
            return "Amazing! You've mastered addition!";
        default:
            return "Great job!";
    }
}
