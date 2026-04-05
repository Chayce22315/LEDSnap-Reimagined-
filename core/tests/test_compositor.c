#include "ls_types.h"
#include "ls_compositor.h"
#include "ls_tools.h"
#include "ls_text.h"
#include "ls_animation.h"
#include "ls_lsnap.h"
#include "ls_frame_limiter.h"
#include <stdio.h>
#include <assert.h>

static int tests_run   = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  %-40s ", #name); \
    tests_run++; \
    if (test_##name()) { tests_passed++; printf("PASS\n"); } \
    else { printf("FAIL\n"); } \
} while(0)

/* ---- tests ---- */

static int test_grid_alloc(void) {
    LSGrid g;
    if (!ls_grid_init(&g, 32, 16)) return 0;
    if (g.width != 32 || g.height != 16) return 0;
    LSPixel *px = ls_grid_at(&g, 0, 0);
    if (px->r != 0 || px->a != 0) return 0; /* calloc should zero */
    ls_grid_free(&g);
    return 1;
}

static int test_compositor_solid(void) {
    LSCompositor comp;
    if (!ls_compositor_init(&comp, 4, 4)) return 0;

    LSPixel red = ls_pixel(255, 0, 0, 255);
    ls_compositor_set_background(&comp, LS_BG_SOLID, red, red);
    ls_compositor_flatten(&comp, 0.016f);

    const LSGrid *out = ls_compositor_get_output(&comp);
    LSPixel p = *ls_grid_at(out, 0, 0);
    int ok = (p.r == 255 && p.g == 0 && p.b == 0 && p.a == 255);

    ls_compositor_destroy(&comp);
    return ok;
}

static int test_compositor_blend(void) {
    LSCompositor comp;
    if (!ls_compositor_init(&comp, 4, 4)) return 0;

    LSPixel blue = ls_pixel(0, 0, 255, 255);
    ls_compositor_set_background(&comp, LS_BG_SOLID, blue, blue);

    /* place a half-transparent green pixel on midground */
    LSPixel green_half = ls_pixel(0, 255, 0, 128);
    *ls_grid_at(&comp.midground, 1, 1) = green_half;

    ls_compositor_flatten(&comp, 0.016f);

    const LSGrid *out = ls_compositor_get_output(&comp);
    LSPixel p = *ls_grid_at(out, 1, 1);

    /* expected: green channel ~128, blue channel ~127 (alpha-over math) */
    int ok = (p.g > 100 && p.b > 100 && p.r < 10);
    ls_compositor_destroy(&comp);
    return ok;
}

static int test_tool_pen(void) {
    LSGrid g;
    if (!ls_grid_init(&g, 8, 8)) return 0;
    LSPixel c = ls_pixel(255, 128, 0, 255);
    ls_tool_pen(&g, 3, 3, c);
    LSPixel p = *ls_grid_at(&g, 3, 3);
    int ok = ls_pixel_eq(p, c);
    ls_grid_free(&g);
    return ok;
}

static int test_tool_eraser(void) {
    LSGrid g;
    if (!ls_grid_init(&g, 8, 8)) return 0;
    *ls_grid_at(&g, 2, 2) = ls_pixel(255, 255, 255, 255);
    ls_tool_eraser(&g, 2, 2);
    LSPixel p = *ls_grid_at(&g, 2, 2);
    int ok = (p.a == 0);
    ls_grid_free(&g);
    return ok;
}

static int test_tool_bucket(void) {
    LSGrid g;
    if (!ls_grid_init(&g, 4, 4)) return 0;
    LSPixel red = ls_pixel(255, 0, 0, 255);
    ls_tool_bucket(&g, 0, 0, red);
    int ok = 1;
    for (int y = 0; y < 4 && ok; y++)
        for (int x = 0; x < 4 && ok; x++)
            ok = ls_pixel_eq(*ls_grid_at(&g, (uint16_t)x, (uint16_t)y), red);
    ls_grid_free(&g);
    return ok;
}

static int test_animation_basic(void) {
    LSAnimation anim;
    if (!ls_animation_init(&anim, 4, 4, 16)) return 0;
    if (anim.frame_count != 1) return 0;

    LSGrid src;
    ls_grid_init(&src, 4, 4);
    *ls_grid_at(&src, 0, 0) = ls_pixel(255, 0, 0, 255);

    if (!ls_animation_add_frame(&anim, &src)) return 0;
    if (anim.frame_count != 2) return 0;

    ls_grid_free(&src);
    ls_animation_destroy(&anim);
    return 1;
}

static int test_lsnap_roundtrip(void) {
    LSSnapFile snap;
    ls_snap_init(&snap, 4, 4, 30, LS_MODE_SOLO);

    snap.palette_size = 2;
    snap.palette[0][0] = 255; snap.palette[0][1] = 0;   snap.palette[0][2] = 0;
    snap.palette[1][0] = 0;   snap.palette[1][1] = 255; snap.palette[1][2] = 0;

    snap.frame_count = 1;
    snap.frames = (LSSnapFrame *)calloc(1, sizeof(LSSnapFrame));
    snap.frames[0].dirty_count = 2;
    snap.frames[0].dirty_pixels = (LSDirtyPixel *)malloc(2 * sizeof(LSDirtyPixel));
    snap.frames[0].dirty_pixels[0] = (LSDirtyPixel){0, 0, 0};
    snap.frames[0].dirty_pixels[1] = (LSDirtyPixel){1, 1, 1};

    const char *tmp = "__test_roundtrip.lsnap";
    if (ls_snap_write(tmp, &snap) != 0) { ls_snap_free(&snap); return 0; }

    LSSnapFile snap2;
    if (ls_snap_read(tmp, &snap2) != 0) { ls_snap_free(&snap); return 0; }

    int ok = (snap2.width == 4 && snap2.height == 4 &&
              snap2.fps == 30 && snap2.palette_size == 2 &&
              snap2.frame_count == 1 &&
              snap2.frames[0].dirty_count == 2);

    ls_snap_free(&snap);
    ls_snap_free(&snap2);
    remove(tmp);
    return ok;
}

static int test_frame_limiter(void) {
    LSFrameLimiter lim;
    ls_limiter_init(&lim, 0);
    if (lim.target_fps != 60.0) return 0;
    ls_limiter_toggle_eco(&lim);
    if (lim.target_fps != 30.0) return 0;
    ls_limiter_toggle_eco(&lim);
    if (lim.target_fps != 60.0) return 0;
    return 1;
}

/* ---- runner ---- */

int main(void) {
    printf("LEDSnap core tests\n");
    printf("==================\n");

    TEST(grid_alloc);
    TEST(compositor_solid);
    TEST(compositor_blend);
    TEST(tool_pen);
    TEST(tool_eraser);
    TEST(tool_bucket);
    TEST(animation_basic);
    TEST(lsnap_roundtrip);
    TEST(frame_limiter);

    printf("==================\n");
    printf("%d / %d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
