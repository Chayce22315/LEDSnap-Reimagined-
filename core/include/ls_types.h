#ifndef LS_TYPES_H
#define LS_TYPES_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t r, g, b, a;
} LSPixel;

typedef struct {
    uint16_t width;
    uint16_t height;
    LSPixel *pixels; /* row-major: pixels[y * width + x] */
} LSGrid;

typedef enum {
    LS_BLEND_NORMAL,
    LS_BLEND_ADDITIVE
} LSBlendMode;

static inline int ls_grid_init(LSGrid *g, uint16_t w, uint16_t h) {
    g->width  = w;
    g->height = h;
    g->pixels = (LSPixel *)calloc((size_t)w * h, sizeof(LSPixel));
    return g->pixels != NULL;
}

static inline void ls_grid_free(LSGrid *g) {
    free(g->pixels);
    g->pixels = NULL;
    g->width  = 0;
    g->height = 0;
}

static inline void ls_grid_clear(LSGrid *g) {
    memset(g->pixels, 0, (size_t)g->width * g->height * sizeof(LSPixel));
}

static inline LSPixel *ls_grid_at(const LSGrid *g, uint16_t x, uint16_t y) {
    return &g->pixels[y * g->width + x];
}

static inline int ls_grid_in_bounds(const LSGrid *g, int x, int y) {
    return x >= 0 && x < g->width && y >= 0 && y < g->height;
}

static inline LSPixel ls_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    LSPixel p = { r, g, b, a };
    return p;
}

static inline int ls_pixel_eq(LSPixel a, LSPixel b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

#define LS_PIXEL_TRANSPARENT ls_pixel(0, 0, 0, 0)
#define LS_PIXEL_BLACK       ls_pixel(0, 0, 0, 255)
#define LS_PIXEL_WHITE       ls_pixel(255, 255, 255, 255)

#endif /* LS_TYPES_H */
