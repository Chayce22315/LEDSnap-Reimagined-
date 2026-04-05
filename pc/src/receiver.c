#include "receiver.h"
#include <stdio.h>

int pc_receiver_load(const char *path, LSSnapFile *snap) {
    int result = ls_snap_read(path, snap);
    if (result != 0)
        fprintf(stderr, "receiver: failed to load '%s'\n", path);
    return result;
}

void pc_receiver_apply_frame(const LSSnapFile *snap, uint32_t frame_idx,
                             LSCompositor *comp) {
    if (frame_idx >= snap->frame_count) return;

    const LSSnapFrame *fr = &snap->frames[frame_idx];
    for (uint32_t i = 0; i < fr->dirty_count; i++) {
        uint16_t x   = fr->dirty_pixels[i].x;
        uint16_t y   = fr->dirty_pixels[i].y;
        uint8_t pidx = fr->dirty_pixels[i].palette_idx;

        if (pidx >= snap->palette_size) continue;
        if (!ls_grid_in_bounds(&comp->midground, x, y)) continue;

        LSPixel color;
        color.r = snap->palette[pidx][0];
        color.g = snap->palette[pidx][1];
        color.b = snap->palette[pidx][2];
        color.a = 255;

        *ls_grid_at(&comp->midground, x, y) = color;
    }
}
