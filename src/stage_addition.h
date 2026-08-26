/* stage_addition.h - Stage 2 content: number bonds & addition strategies.
 *
 * Four ordered steps, cumulative and interleaved exactly like Stage 1 (see
 * stage_subitizing.h for the rationale): number bonds to 10, counting-on,
 * doubles/near-doubles, then make-ten/bridging. See IMPLEMENTATION_PLAN.md
 * Stage 2.
 */
#ifndef STAGE_ADDITION_H
#define STAGE_ADDITION_H

#include <stddef.h>

typedef enum {
    ADD_SMALL_SUM,
    ADD_BONDS_TO_10,
    ADD_COUNTING_ON,
    ADD_DOUBLES,
    ADD_MAKE_TEN,
} AdditionFactKind;

typedef struct {
    AdditionFactKind kind;
    int a, b;   /* as shown; for ADD_BONDS_TO_10, b is the unknown addend */
    int answer; /* what the child must type */
} AdditionFact;

typedef enum {
    ASTEP_SMALL_SUMS, /* plain a+b, both addends 1-5: the on-ramp into
                        * symbolic addition before any named strategy - an
                        * unknown-addend problem like bonds-to-10 is a
                        * harder starting point than a straightforward sum. */
    ASTEP_BONDS_TO_10,
    ASTEP_COUNTING_ON,
    ASTEP_DOUBLES,
    ASTEP_MAKE_TEN,
    ASTEP_DONE,
} AdditionStep;

#define ADDITION_MAX_FACTS 63 /* 25 + 9 + 12 + 9 + 8 */

int addition_step_new_fact_count(AdditionStep step);
int addition_step_start_index(AdditionStep step);
int addition_build_pool(AdditionStep upto_step, AdditionFact out[ADDITION_MAX_FACTS]);
const char *addition_step_intro_message(AdditionStep step);

/* True for strategies with a supporting diagram (counting-on's number
 * line, make-ten's ten-frame), false for the plain-equation ones (small
 * sums, bonds, doubles). The caller uses this to decide where the equation
 * text goes: below a diagram if there is one, or vertically centered in
 * the play area if there isn't - a fact with no diagram shouldn't render
 * its equation stranded at the same low position a diagram would leave
 * room above for. */
int addition_fact_has_diagram(AdditionFactKind kind);

/* Draws the fact's supporting diagram, if its strategy has one (counting-on
 * gets a number line, make-ten gets a ten-frame; bonds and doubles have
 * none), and writes the equation text to display alongside it into buf.
 * Unlike Stage 1's subitizing dots, this is fully deterministic given the
 * fact - no randomness - so it's safe to call on every redraw without
 * caching. Depends on viewport.h; kept in a separate translation unit
 * (stage_addition_draw.c) so host-only unit tests can link the pure logic
 * above without pulling in a platform backend. */
void addition_draw_fact(const AdditionFact *fact, char *buf, size_t buf_size);

#endif
