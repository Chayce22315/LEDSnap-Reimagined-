#ifndef PC_RENDERER_H
#define PC_RENDERER_H

#include "ls_types.h"
#include <SDL2/SDL.h>

typedef struct {
    SDL_Renderer *sdl_renderer;
    SDL_Texture  *texture;
    int pixel_scale;   /* each grid cell = pixel_scale x pixel_scale screen pixels */
    uint16_t grid_w;
    uint16_t grid_h;
} PCRenderer;

int  pc_renderer_init(PCRenderer *r, SDL_Renderer *sdl, uint16_t gw, uint16_t gh, int scale);
void pc_renderer_destroy(PCRenderer *r);
void pc_renderer_present(PCRenderer *r, const LSGrid *output);

#endif /* PC_RENDERER_H */
