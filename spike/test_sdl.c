/*
 * test_sdl.c - throwaway SDL2 feasibility spike.
 *
 * Opens a window, fills it with a solid color, draws one filled
 * rectangle, then exits automatically after ~3 seconds (or sooner on
 * a keypress / ESC / window-close), so it terminates unattended for
 * automated testing.
 *
 * Uses only plain, portable SDL2 APIs (SDL_CreateWindow, SDL_Renderer,
 * SDL_RenderFillRect, etc.) -- no platform-specific extensions -- so
 * this exact source can be rebuilt unmodified on the Raspberry Pi
 * with its own SDL2/gcc.
 */

#include <SDL2/SDL.h>
#include <stdio.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480
#define RUN_MS        3000 /* auto-exit after this many milliseconds */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "SDL2 Spike",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL) {
        /* Fall back to software rendering if accelerated isn't available
         * (expected on the Pi's legacy framebuffer with no GPU path). */
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Uint32 start_ticks = SDL_GetTicks();
    int running = 1;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                /* Any keypress (including ESC) exits early. */
                running = 0;
            }
        }

        if (SDL_GetTicks() - start_ticks >= RUN_MS) {
            running = 0;
        }

        /* Fill background with a solid color. */
        SDL_SetRenderDrawColor(renderer, 30, 60, 120, 255);
        SDL_RenderClear(renderer);

        /* Draw one filled rectangle. */
        SDL_Rect rect;
        rect.w = 200;
        rect.h = 120;
        rect.x = (WINDOW_WIDTH - rect.w) / 2;
        rect.y = (WINDOW_HEIGHT - rect.h) / 2;
        SDL_SetRenderDrawColor(renderer, 240, 200, 40, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); /* ~60 fps */
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("test_sdl: exited cleanly\n");
    return 0;
}
