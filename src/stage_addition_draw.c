/* stage_addition_draw.c - visual rendering for Stage 2 facts.
 * Pure step/pool logic lives in stage_addition.c (host-testable, no
 * viewport dependency); this file is the platform/game-loop side.
 */
#include "stage_addition.h"

#include <stdio.h>

#include "viewport.h"

/* Number line for counting-on: shows the starting point (the larger
 * addend) and `smaller`-many hop markers stepping right from it, but never
 * labels where they land - the child still has to work out the sum. */
#define NUMBERLINE_Y 260
#define NUMBERLINE_X0 100
#define NUMBERLINE_X1 900
#define NUMBERLINE_MAX 18

static const Color kLineColor = {150, 150, 170};
static const Color kMarkerColor = {90, 170, 240};
static const Color kHopColor = {240, 170, 60};

static int tick_x(int v) {
    return NUMBERLINE_X0 + v * (NUMBERLINE_X1 - NUMBERLINE_X0) / NUMBERLINE_MAX;
}

static void draw_number_line(int larger, int smaller) {
    vdraw_thick_line(NUMBERLINE_X0, NUMBERLINE_Y, NUMBERLINE_X1, NUMBERLINE_Y,
                      6, kLineColor);
    for (int v = 0; v <= NUMBERLINE_MAX; v++) {
        int x = tick_x(v);
        vfill_rect(x - 2, NUMBERLINE_Y - 10, 4, 20, kLineColor);
    }
    int mx = tick_x(larger);
    vfill_rect(mx - 14, NUMBERLINE_Y - 14, 28, 28, kMarkerColor);
    for (int i = 1; i <= smaller; i++) {
        int hx = tick_x(larger + i);
        vfill_rect(hx - 10, NUMBERLINE_Y - 46, 20, 20, kHopColor);
    }
}

/* Ten-frame for make-ten: `a` filled cells out of 10 show at a glance how
 * many more are needed to complete the frame (10 - a); the second addend
 * `b` is shown separately as loose squares, for the child to mentally move
 * (10 - a) of them into the frame and see what's left over. */
#define TEN_FRAME_X0 260
#define TEN_FRAME_Y0 140
#define CELL 60
#define CELL_GAP 8
#define EXTRA_DOT 40
#define EXTRA_GAP 12

static const Color kFilledColor = {90, 170, 240};
static const Color kEmptyColor = {60, 60, 90};
static const Color kExtraColor = {240, 170, 60};

static void draw_ten_frame(int a, int b) {
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 5; col++) {
            int i = row * 5 + col;
            int x = TEN_FRAME_X0 + col * (CELL + CELL_GAP);
            int y = TEN_FRAME_Y0 + row * (CELL + CELL_GAP);
            vfill_rect(x, y, CELL, CELL, i < a ? kFilledColor : kEmptyColor);
        }
    }

    int extra_x0 = TEN_FRAME_X0 + 5 * (CELL + CELL_GAP) + 60;
    for (int i = 0; i < b; i++) {
        int col = i % 5;
        int row = i / 5;
        int x = extra_x0 + col * (EXTRA_DOT + EXTRA_GAP);
        int y = TEN_FRAME_Y0 + row * (EXTRA_DOT + EXTRA_GAP);
        vfill_rect(x, y, EXTRA_DOT, EXTRA_DOT, kExtraColor);
    }
}

void addition_draw_fact(const AdditionFact *fact, char *buf, size_t buf_size) {
    switch (fact->kind) {
        case ADD_SMALL_SUM:
            snprintf(buf, buf_size, "%d + %d = ?", fact->a, fact->b);
            break;
        case ADD_BONDS_TO_10:
            snprintf(buf, buf_size, "%d + ? = 10", fact->a);
            break;
        case ADD_COUNTING_ON:
            draw_number_line(fact->a, fact->b);
            snprintf(buf, buf_size, "%d + %d = ?", fact->a, fact->b);
            break;
        case ADD_DOUBLES:
            snprintf(buf, buf_size, "%d + %d = ?", fact->a, fact->b);
            break;
        case ADD_MAKE_TEN:
            draw_ten_frame(fact->a, fact->b);
            snprintf(buf, buf_size, "%d + %d = ?", fact->a, fact->b);
            break;
    }
}
