#ifndef LS_LSNAP_H
#define LS_LSNAP_H

#include "ls_types.h"
#include "ls_animation.h"
#include <stdio.h>

/*
 * .lsnap binary format v1
 *
 * HEADER (16 bytes)
 *   [0..3]   magic        "LSNP"
 *   [4..5]   version      uint16  (1)
 *   [6..7]   width        uint16
 *   [8..9]   height       uint16
 *   [10..11] fps          uint16
 *   [12..13] mode_flag    uint16  (0=solo, 1=linked)
 *   [14..15] palette_size uint16  (max 256)
 *
 * PALETTE (palette_size * 3 bytes)
 *   Each entry: R, G, B (1 byte each)
 *
 * FRAMES
 *   [+0..3]  frame_count  uint32
 *   Per frame:
 *     [+0..3]  dirty_count  uint32
 *     Per dirty pixel:
 *       x             uint16
 *       y             uint16
 *       palette_idx   uint8
 */

#define LS_LSNAP_MAGIC       "LSNP"
#define LS_LSNAP_VERSION     1
#define LS_LSNAP_MAX_PALETTE 256

typedef enum {
    LS_MODE_SOLO   = 0,
    LS_MODE_LINKED = 1
} LSModeFlag;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  palette_idx;
} LSDirtyPixel;

typedef struct {
    uint32_t      dirty_count;
    LSDirtyPixel *dirty_pixels;
} LSSnapFrame;

typedef struct {
    /* header */
    uint16_t   version;
    uint16_t   width;
    uint16_t   height;
    uint16_t   fps;
    LSModeFlag mode_flag;

    /* palette */
    uint16_t palette_size;
    uint8_t  palette[LS_LSNAP_MAX_PALETTE][3]; /* RGB */

    /* frames */
    uint32_t    frame_count;
    LSSnapFrame *frames;
} LSSnapFile;

void ls_snap_init(LSSnapFile *snap, uint16_t w, uint16_t h,
                  uint16_t fps, LSModeFlag mode);
void ls_snap_free(LSSnapFile *snap);

/*
 * Build palette from animation frames.
 * Naive unique-color collector (stub for future median-cut quantization).
 * Returns 0 on success, -1 if >256 unique colors.
 */
int ls_snap_build_palette(LSSnapFile *snap, const LSAnimation *anim);

/*
 * Compute dirty-pixel delta between two grids.
 * Writes results into caller-allocated arrays.
 * Returns the number of dirty pixels found.
 */
uint32_t ls_snap_diff_frame(const LSGrid *prev, const LSGrid *cur,
                            LSDirtyPixel *out, uint32_t out_capacity,
                            const LSSnapFile *snap);

/* Serialize a fully-populated LSSnapFile to disk. Returns 0 on success. */
int ls_snap_write(const char *path, const LSSnapFile *snap);

/* Deserialize from disk. Caller must ls_snap_free() when done. Returns 0 on success. */
int ls_snap_read(const char *path, LSSnapFile *snap);

/* Find palette index for an RGB triplet. Returns index or -1 if not found. */
int ls_snap_palette_find(const LSSnapFile *snap, uint8_t r, uint8_t g, uint8_t b);

#endif /* LS_LSNAP_H */
