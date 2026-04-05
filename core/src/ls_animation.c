#include "ls_animation.h"
#include <string.h>

#define ONION_ALPHA 77 /* ~30% of 255 */

int ls_animation_init(LSAnimation *anim, uint16_t w, uint16_t h, uint16_t max_frames) {
    memset(anim, 0, sizeof(*anim));
    anim->width      = w;
    anim->height     = h;
    anim->max_frames = max_frames;
    anim->onion_skin_enabled = 0;

    anim->frames = (LSGrid *)calloc(max_frames, sizeof(LSGrid));
    if (!anim->frames) return 0;

    /* Pre-allocate the first frame as a blank canvas */
    if (!ls_grid_init(&anim->frames[0], w, h)) {
        free(anim->frames);
        anim->frames = NULL;
        return 0;
    }
    anim->frame_count   = 1;
    anim->current_frame = 0;
    return 1;
}

void ls_animation_destroy(LSAnimation *anim) {
    if (anim->frames) {
        for (uint16_t i = 0; i < anim->frame_count; i++)
            ls_grid_free(&anim->frames[i]);
        free(anim->frames);
        anim->frames = NULL;
    }
    anim->frame_count   = 0;
    anim->current_frame = 0;
}

int ls_animation_add_frame(LSAnimation *anim, const LSGrid *source) {
    if (anim->frame_count >= anim->max_frames) return 0;

    uint16_t idx = anim->frame_count;
    if (!ls_grid_init(&anim->frames[idx], anim->width, anim->height))
        return 0;

    if (source && source->pixels) {
        memcpy(anim->frames[idx].pixels, source->pixels,
               (size_t)anim->width * anim->height * sizeof(LSPixel));
    }

    anim->frame_count++;
    anim->current_frame = idx;
    return 1;
}

void ls_animation_set_frame(LSAnimation *anim, uint16_t index) {
    if (index < anim->frame_count)
        anim->current_frame = index;
}

void ls_animation_next_frame(LSAnimation *anim) {
    if (anim->current_frame + 1 < anim->frame_count)
        anim->current_frame++;
    else
        anim->current_frame = 0;
}

void ls_animation_prev_frame(LSAnimation *anim) {
    if (anim->current_frame > 0)
        anim->current_frame--;
    else
        anim->current_frame = anim->frame_count - 1;
}

const LSGrid *ls_animation_current(const LSAnimation *anim) {
    if (anim->frame_count == 0) return NULL;
    return &anim->frames[anim->current_frame];
}

int ls_animation_get_onion(const LSAnimation *anim, LSGrid *out) {
    if (!anim->onion_skin_enabled) return 0;
    if (anim->frame_count < 2)     return 0;

    uint16_t prev = (anim->current_frame > 0)
                        ? anim->current_frame - 1
                        : anim->frame_count - 1;

    const LSGrid *prev_frame = &anim->frames[prev];
    size_t total = (size_t)anim->width * anim->height;

    for (size_t i = 0; i < total; i++) {
        LSPixel p = prev_frame->pixels[i];
        p.a = (uint8_t)((p.a * ONION_ALPHA) / 255);
        out->pixels[i] = p;
    }
    return 1;
}
