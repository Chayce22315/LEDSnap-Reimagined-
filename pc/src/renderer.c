#include "renderer.h"

int pc_renderer_init(PCRenderer *r, SDL_Renderer *sdl,
                     uint16_t gw, uint16_t gh, int scale) {
    r->sdl_renderer = sdl;
    r->grid_w      = gw;
    r->grid_h      = gh;
    r->pixel_scale = scale;

    r->texture = SDL_CreateTexture(
        sdl,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        gw, gh
    );
    return r->texture != NULL;
}

void pc_renderer_destroy(PCRenderer *r) {
    if (r->texture) {
        SDL_DestroyTexture(r->texture);
        r->texture = NULL;
    }
}

void pc_renderer_present(PCRenderer *r, const LSGrid *output) {
    void *tex_pixels;
    int pitch;

    if (SDL_LockTexture(r->texture, NULL, &tex_pixels, &pitch) != 0)
        return;

    /* Copy grid pixels into the texture row by row */
    for (int y = 0; y < r->grid_h; y++) {
        uint8_t *dst = (uint8_t *)tex_pixels + y * pitch;
        const LSPixel *src = &output->pixels[y * r->grid_w];
        for (int x = 0; x < r->grid_w; x++) {
            dst[x * 4 + 0] = src[x].r;
            dst[x * 4 + 1] = src[x].g;
            dst[x * 4 + 2] = src[x].b;
            dst[x * 4 + 3] = src[x].a;
        }
    }

    SDL_UnlockTexture(r->texture);

    SDL_RenderClear(r->sdl_renderer);
    SDL_RenderCopy(r->sdl_renderer, r->texture, NULL, NULL);
    SDL_RenderPresent(r->sdl_renderer);
}
