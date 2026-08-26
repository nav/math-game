/* stage_subitizing_draw.c - visual rendering for Stage 1 facts.
 * Pure step/pool logic lives in stage_subitizing.c (host-testable, no
 * viewport dependency); this file is the platform/game-loop side.
 */
#include "stage_subitizing.h"

#include <stdlib.h>

#include "viewport.h"

#define DOT_SIZE 56
#define JITTER 8

#define PERCEPTUAL_BOX_HALF 130
#define COMPOSED_BOX_HALF 65
#define COMPOSED_CLUSTER_GAP 150

#define DOTS_CY 220

/* Canonical dice-face layouts, as fractional (x, y) offsets from center in
 * [-1, 1]. Index by quantity - 1; covers 1-5, which is all any single
 * cluster ever needs (a composed fact is at most two 5-clusters). */
static const double kLayouts[5][5][2] = {
    /* 1 */ {{0, 0}},
    /* 2 */ {{-1, -1}, {1, 1}},
    /* 3 */ {{-1, -1}, {0, 0}, {1, 1}},
    /* 4 */ {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}},
    /* 5 */ {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}},
};

static const Color kDotColor = {90, 170, 240};

static int jitter(void) { return (rand() % (2 * JITTER + 1)) - JITTER; }

/* Appends n dots in a canonical dice-face pattern, centered at (cx, cy),
 * to out starting at *idx. */
static void layout_pattern_at(SubitizeLayout *out, int *idx, int n, int cx,
                               int cy, int box_half) {
    if (n < 1 || n > 5) return;
    const double(*layout)[2] = kLayouts[n - 1];
    for (int i = 0; i < n; i++) {
        out->x[*idx] = cx + (int)(layout[i][0] * box_half) + jitter();
        out->y[*idx] = cy + (int)(layout[i][1] * box_half) + jitter();
        (*idx)++;
    }
}

/* Scattered (non-canonical) layout for counting: dots must be individually
 * countable rather than instantly recognizable as a pattern, so children
 * practice the actual counting sequence instead of pattern-matching. Picks
 * `n` distinct cells from a fixed grid, spaced further apart than DOT_SIZE
 * so dots never overlap even with jitter. */
#define GRID_COLS 5
#define GRID_ROWS 3
#define GRID_CELL_W 150
#define GRID_CELL_H 110

static void layout_scattered(SubitizeLayout *out, int n) {
    if (n < 1 || n > GRID_COLS * GRID_ROWS) return;

    int cells[GRID_COLS * GRID_ROWS];
    int num_cells = GRID_COLS * GRID_ROWS;
    for (int i = 0; i < num_cells; i++) cells[i] = i;

    /* Partial Fisher-Yates: only need the first n picks to be random. */
    for (int i = 0; i < n; i++) {
        int j = i + rand() % (num_cells - i);
        int tmp = cells[i];
        cells[i] = cells[j];
        cells[j] = tmp;
    }

    int grid_w = (GRID_COLS - 1) * GRID_CELL_W;
    int grid_h = (GRID_ROWS - 1) * GRID_CELL_H;
    int origin_x = VIRTUAL_W / 2 - grid_w / 2;
    int origin_y = DOTS_CY - grid_h / 2;

    for (int i = 0; i < n; i++) {
        int col = cells[i] % GRID_COLS;
        int row = cells[i] / GRID_COLS;
        out->x[i] = origin_x + col * GRID_CELL_W + jitter();
        out->y[i] = origin_y + row * GRID_CELL_H + jitter();
    }
    out->count = n;
}

void subitize_compute_layout(const SubitizeFact *fact, SubitizeLayout *out) {
    out->count = 0;
    switch (fact->kind) {
        case SFK_PERCEPTUAL: {
            int idx = 0;
            layout_pattern_at(out, &idx, fact->quantity, VIRTUAL_W / 2,
                               DOTS_CY, PERCEPTUAL_BOX_HALF);
            out->count = idx;
            out->prompt = "How many?";
            break;
        }
        case SFK_COMPOSED: {
            int idx = 0;
            int extra = fact->quantity - 5;
            layout_pattern_at(out, &idx, 5, VIRTUAL_W / 2 - COMPOSED_CLUSTER_GAP,
                               DOTS_CY, COMPOSED_BOX_HALF);
            layout_pattern_at(out, &idx, extra,
                               VIRTUAL_W / 2 + COMPOSED_CLUSTER_GAP, DOTS_CY,
                               COMPOSED_BOX_HALF);
            out->count = idx;
            out->prompt = "How many?";
            break;
        }
        case SFK_COUNT:
            layout_scattered(out, fact->quantity);
            out->prompt = "How many in total?";
            break;
    }
}

void subitize_draw_layout(const SubitizeLayout *layout) {
    for (int i = 0; i < layout->count; i++) {
        vfill_rect(layout->x[i] - DOT_SIZE / 2, layout->y[i] - DOT_SIZE / 2,
                   DOT_SIZE, DOT_SIZE, kDotColor);
    }
}
