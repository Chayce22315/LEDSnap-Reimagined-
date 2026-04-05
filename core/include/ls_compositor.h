#ifndef LS_COMPOSITOR_H
#define LS_COMPOSITOR_H

#include "ls_types.h"
#include <math.h>

typedef enum {
    LS_BG_SOLID,
    LS_BG_GRADIENT,
    LS_BG_RAINBOW
} LSBackgroundMode;

typedef struct {
    LSGrid background;
    LSGrid midground;
    LSGrid foreground;
    LSGrid output;

    LSBackgroundMode bg_mode;
    LSPixel bg_color_a; /* solid color / gradient start */
    LSPixel bg_color_b; /* gradient end */
    float   bg_rainbow_phase;
    float   bg_rainbow_speed; /* phase advance per second */

    uint16_t width;
    uint16_t height;
} LSCompositor;

int  ls_compositor_init(LSCompositor *comp, uint16_t w, uint16_t h);
void ls_compositor_destroy(LSCompositor *comp);

void ls_compositor_set_background(LSCompositor *comp,
                                  LSBackgroundMode mode,
                                  LSPixel color_a,
                                  LSPixel color_b);

void ls_compositor_flatten(LSCompositor *comp, float delta_time);

const LSGrid *ls_compositor_get_output(const LSCompositor *comp);

/* Blend helpers exposed for reuse by animation/onion-skin code */
static inline LSPixel ls_alpha_over(LSPixel dst, LSPixel src) {
    uint16_t sa = src.a;
    uint16_t da = 255 - sa;
    LSPixel out;
    out.r = (uint8_t)((src.r * sa + dst.r * da) / 255);
    out.g = (uint8_t)((src.g * sa + dst.g * da) / 255);
    out.b = (uint8_t)((src.b * sa + dst.b * da) / 255);
    out.a = (uint8_t)(sa + (dst.a * da) / 255);
    return out;
}

static inline LSPixel ls_additive_blend(LSPixel dst, LSPixel src) {
    LSPixel out;
    uint16_t r = (uint16_t)dst.r + src.r;
    uint16_t g = (uint16_t)dst.g + src.g;
    uint16_t b = (uint16_t)dst.b + src.b;
    out.r = (uint8_t)(r > 255 ? 255 : r);
    out.g = (uint8_t)(g > 255 ? 255 : g);
    out.b = (uint8_t)(b > 255 ? 255 : b);
    out.a = dst.a > src.a ? dst.a : src.a;
    return out;
}

#endif /* LS_COMPOSITOR_H */
