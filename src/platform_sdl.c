/* platform_sdl.c - macOS local dev backend ONLY. Never runs on the Pi.
 *
 * Exists purely so game.c can be iterated on quickly in a real window via
 * `nix-shell` before syncing to the Pi, which uses platform_fbdev.c
 * instead (this board has no working SDL2 video driver at all).
 */
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "platform.h"

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;

/* Deliberately different aspect ratio from the Pi's 1920x1200 (8:5), to
 * exercise letterboxing locally too. */
#define DEV_WINDOW_W 800
#define DEV_WINDOW_H 600

int plat_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    g_window = SDL_CreateWindow("math-game (dev)", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, DEV_WINDOW_W,
                                 DEV_WINDOW_H, SDL_WINDOW_SHOWN);
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) {
        g_renderer =
            SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!g_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

void plat_shutdown(void) {
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void plat_get_screen_size(int *w, int *h) {
    SDL_GetRendererOutputSize(g_renderer, w, h);
}

void plat_fill_rect_px(int x, int y, int w, int h, Color c) {
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, 255);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(g_renderer, &r);
}

void plat_blit_alpha(int x, int y, int w, int h, const unsigned char *alpha,
                      Color c) {
    SDL_Surface *surf =
        SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;
    SDL_LockSurface(surf);
    uint32_t *px = (uint32_t *)surf->pixels;
    for (int i = 0; i < w * h; i++) {
        px[i] = SDL_MapRGBA(surf->format, c.r, c.g, c.b, alpha[i]);
    }
    SDL_UnlockSurface(surf);

    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_Rect dst = {x, y, w, h};
        SDL_RenderCopy(g_renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void plat_present(void) { SDL_RenderPresent(g_renderer); }

Key plat_poll_key(void) {
    SDL_Event ev;
    if (!SDL_PollEvent(&ev)) return GK_NONE;

    if (ev.type == SDL_QUIT) return GK_ESCAPE;
    if (ev.type != SDL_KEYDOWN) return GK_NONE;

    switch (ev.key.keysym.scancode) {
        case SDL_SCANCODE_0: return GK_0;
        case SDL_SCANCODE_1: return GK_1;
        case SDL_SCANCODE_2: return GK_2;
        case SDL_SCANCODE_3: return GK_3;
        case SDL_SCANCODE_4: return GK_4;
        case SDL_SCANCODE_5: return GK_5;
        case SDL_SCANCODE_6: return GK_6;
        case SDL_SCANCODE_7: return GK_7;
        case SDL_SCANCODE_8: return GK_8;
        case SDL_SCANCODE_9: return GK_9;
        case SDL_SCANCODE_RETURN: return GK_ENTER;
        case SDL_SCANCODE_BACKSPACE: return GK_BACKSPACE;
        case SDL_SCANCODE_ESCAPE: return GK_ESCAPE;
        default: return GK_NONE;
    }
}

long plat_time_ms(void) { return (long)SDL_GetTicks(); }
