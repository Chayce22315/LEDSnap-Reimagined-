#ifndef LS_ANIMATION_H
#define LS_ANIMATION_H

#include "ls_types.h"

typedef struct {
    LSGrid  *frames;
    uint16_t frame_count;
    uint16_t current_frame;
    uint16_t max_frames;
    int      onion_skin_enabled;
    uint16_t width;
    uint16_t height;
} LSAnimation;

int  ls_animation_init(LSAnimation *anim, uint16_t w, uint16_t h, uint16_t max_frames);
void ls_animation_destroy(LSAnimation *anim);

int  ls_animation_add_frame(LSAnimation *anim, const LSGrid *source);
void ls_animation_set_frame(LSAnimation *anim, uint16_t index);
void ls_animation_next_frame(LSAnimation *anim);
void ls_animation_prev_frame(LSAnimation *anim);

const LSGrid *ls_animation_current(const LSAnimation *anim);

/*
 * Writes a blended onion-skin ghost of the previous frame into `out`.
 * The ghost is the previous frame at ~30% opacity.
 * `out` must already be initialized to the correct dimensions.
 */
int ls_animation_get_onion(const LSAnimation *anim, LSGrid *out);

#endif /* LS_ANIMATION_H */
