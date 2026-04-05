#ifndef PC_RECEIVER_H
#define PC_RECEIVER_H

#include "ls_lsnap.h"
#include "ls_compositor.h"

/*
 * Load a .lsnap file and reconstruct animation frames into a compositor.
 * Returns 0 on success.
 */
int pc_receiver_load(const char *path, LSSnapFile *snap);

/*
 * Apply a single frame's dirty pixels from the snap file onto a compositor's
 * midground layer using the embedded palette.
 */
void pc_receiver_apply_frame(const LSSnapFile *snap, uint32_t frame_idx,
                             LSCompositor *comp);

#endif /* PC_RECEIVER_H */
