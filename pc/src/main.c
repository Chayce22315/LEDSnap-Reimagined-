#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "ls_compositor.h"
#include "ls_text.h"
#include "ls_frame_limiter.h"
#include "ls_lsnap.h"
#include "renderer.h"
#include "receiver.h"

#define DEFAULT_GRID_W 64
#define DEFAULT_GRID_H 16
#define DEFAULT_SCALE  12

int main(int argc, char *argv[]) {
    uint16_t gw    = DEFAULT_GRID_W;
    uint16_t gh    = DEFAULT_GRID_H;
    int      scale = DEFAULT_SCALE;
    int      eco   = 0;
    const char *lsnap_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--eco") == 0) {
            eco = 1;
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            scale = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            gw = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            gh = (uint16_t)atoi(argv[++i]);
        } else {
            lsnap_path = argv[i];
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    int win_w = gw * scale;
    int win_h = gh * scale;

    SDL_Window *window = SDL_CreateWindow(
        "LEDSnap! Receiver",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!sdl_renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* Core engine */
    LSCompositor comp;
    ls_compositor_init(&comp, gw, gh);
    ls_compositor_set_background(&comp, LS_BG_RAINBOW,
                                 ls_pixel(0, 0, 0, 255),
                                 ls_pixel(0, 0, 0, 255));

    LSFrameLimiter limiter;
    ls_limiter_init(&limiter, eco);

    /* Scrolling demo text (used when no .lsnap file is loaded) */
    LSTextElement text_elem;
    ls_text_init(&text_elem, "LEDSNAP! RECEIVER READY", -30.0f,
                 ls_pixel(255, 255, 255, 255), (gh - LS_FONT_CHAR_H) / 2);
    text_elem.x_offset = (float)gw;

    /* Optional .lsnap playback */
    LSSnapFile snap;
    int snap_loaded = 0;
    uint32_t snap_frame = 0;
    double snap_timer = 0.0;

    if (lsnap_path) {
        if (pc_receiver_load(lsnap_path, &snap) == 0) {
            snap_loaded = 1;
            gw = snap.width;
            gh = snap.height;
        }
    }

    /* Renderer */
    PCRenderer renderer;
    pc_renderer_init(&renderer, sdl_renderer, gw, gh, scale);

    /* Main loop */
    int running = 1;
    while (running) {
        ls_limiter_begin(&limiter);

        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT)
                running = 0;
            if (evt.type == SDL_KEYDOWN) {
                if (evt.key.keysym.sym == SDLK_ESCAPE)
                    running = 0;
                if (evt.key.keysym.sym == SDLK_e)
                    ls_limiter_toggle_eco(&limiter);
            }
        }

        float dt = (float)limiter.delta_time;

        if (snap_loaded) {
            snap_timer += dt;
            double frame_dur = 1.0 / (snap.fps > 0 ? snap.fps : 30);
            while (snap_timer >= frame_dur) {
                snap_timer -= frame_dur;
                snap_frame = (snap_frame + 1) % snap.frame_count;
            }
            ls_grid_clear(&comp.midground);
            pc_receiver_apply_frame(&snap, snap_frame, &comp);
        } else {
            ls_text_update(&text_elem, dt, gw);
            ls_grid_clear(&comp.foreground);
            ls_text_render(&text_elem, &comp.foreground);
        }

        ls_compositor_flatten(&comp, dt);
        pc_renderer_present(&renderer, ls_compositor_get_output(&comp));

        ls_limiter_end(&limiter);
    }

    /* Cleanup */
    pc_renderer_destroy(&renderer);
    ls_compositor_destroy(&comp);
    if (snap_loaded) ls_snap_free(&snap);

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
