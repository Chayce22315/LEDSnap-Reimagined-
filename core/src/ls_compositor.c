#include "ls_compositor.h"

int ls_compositor_init(LSCompositor *comp, uint16_t w, uint16_t h) {
    memset(comp, 0, sizeof(*comp));
    comp->width  = w;
    comp->height = h;
    comp->bg_mode = LS_BG_SOLID;
    comp->bg_color_a = LS_PIXEL_BLACK;
    comp->bg_color_b = LS_PIXEL_BLACK;
    comp->bg_rainbow_phase = 0.0f;
    comp->bg_rainbow_speed = 1.0f;

    if (!ls_grid_init(&comp->background, w, h)) return 0;
    if (!ls_grid_init(&comp->midground,  w, h)) return 0;
    if (!ls_grid_init(&comp->foreground, w, h)) return 0;
    if (!ls_grid_init(&comp->output,     w, h)) return 0;
    return 1;
}

void ls_compositor_destroy(LSCompositor *comp) {
    ls_grid_free(&comp->background);
    ls_grid_free(&comp->midground);
    ls_grid_free(&comp->foreground);
    ls_grid_free(&comp->output);
}

void ls_compositor_set_background(LSCompositor *comp,
                                  LSBackgroundMode mode,
                                  LSPixel color_a,
                                  LSPixel color_b) {
    comp->bg_mode    = mode;
    comp->bg_color_a = color_a;
    comp->bg_color_b = color_b;
}

/* --- background generators --- */

static void bg_fill_solid(LSCompositor *comp) {
    size_t total = (size_t)comp->width * comp->height;
    for (size_t i = 0; i < total; i++)
        comp->background.pixels[i] = comp->bg_color_a;
}

static uint8_t lerp8(uint8_t a, uint8_t b, float t) {
    return (uint8_t)(a + (b - a) * t);
}

static void bg_fill_gradient(LSCompositor *comp) {
    for (uint16_t x = 0; x < comp->width; x++) {
        float t = (comp->width > 1) ? (float)x / (comp->width - 1) : 0.0f;
        LSPixel col;
        col.r = lerp8(comp->bg_color_a.r, comp->bg_color_b.r, t);
        col.g = lerp8(comp->bg_color_a.g, comp->bg_color_b.g, t);
        col.b = lerp8(comp->bg_color_a.b, comp->bg_color_b.b, t);
        col.a = 255;
        for (uint16_t y = 0; y < comp->height; y++)
            comp->background.pixels[y * comp->width + x] = col;
    }
}

static LSPixel hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r1, g1, b1;
    if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }
    LSPixel p;
    p.r = (uint8_t)((r1 + m) * 255.0f);
    p.g = (uint8_t)((g1 + m) * 255.0f);
    p.b = (uint8_t)((b1 + m) * 255.0f);
    p.a = 255;
    return p;
}

static void bg_fill_rainbow(LSCompositor *comp) {
    for (uint16_t x = 0; x < comp->width; x++) {
        float hue = fmodf(comp->bg_rainbow_phase * 360.0f +
                          (float)x / comp->width * 360.0f, 360.0f);
        if (hue < 0) hue += 360.0f;
        LSPixel col = hsv_to_rgb(hue, 1.0f, 1.0f);
        for (uint16_t y = 0; y < comp->height; y++)
            comp->background.pixels[y * comp->width + x] = col;
    }
}

/* --- main flatten --- */

void ls_compositor_flatten(LSCompositor *comp, float delta_time) {
    /* 1. Generate background */
    switch (comp->bg_mode) {
        case LS_BG_SOLID:    bg_fill_solid(comp);    break;
        case LS_BG_GRADIENT: bg_fill_gradient(comp);  break;
        case LS_BG_RAINBOW:
            comp->bg_rainbow_phase += comp->bg_rainbow_speed * delta_time;
            if (comp->bg_rainbow_phase > 1.0f) comp->bg_rainbow_phase -= 1.0f;
            if (comp->bg_rainbow_phase < 0.0f) comp->bg_rainbow_phase += 1.0f;
            bg_fill_rainbow(comp);
            break;
    }

    /* 2. Composite: output = fg OVER (mg OVER bg) */
    size_t total = (size_t)comp->width * comp->height;
    for (size_t i = 0; i < total; i++) {
        LSPixel base = comp->background.pixels[i];
        LSPixel mid  = comp->midground.pixels[i];
        LSPixel fg   = comp->foreground.pixels[i];

        LSPixel pass1 = ls_alpha_over(base, mid);
        LSPixel pass2 = ls_alpha_over(pass1, fg);
        comp->output.pixels[i] = pass2;
    }
}

const LSGrid *ls_compositor_get_output(const LSCompositor *comp) {
    return &comp->output;
}
