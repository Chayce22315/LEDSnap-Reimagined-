#include "ls_lsnap.h"
#include <string.h>

/* ---- helpers for little-endian I/O ---- */

static void write_u16(FILE *f, uint16_t v) {
    uint8_t buf[2] = { (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
    fwrite(buf, 1, 2, f);
}
static void write_u32(FILE *f, uint32_t v) {
    uint8_t buf[4] = {
        (uint8_t)(v),       (uint8_t)(v >> 8),
        (uint8_t)(v >> 16), (uint8_t)(v >> 24)
    };
    fwrite(buf, 1, 4, f);
}
static uint16_t read_u16(FILE *f) {
    uint8_t buf[2];
    fread(buf, 1, 2, f);
    return (uint16_t)(buf[0] | (buf[1] << 8));
}
static uint32_t read_u32(FILE *f) {
    uint8_t buf[4];
    fread(buf, 1, 4, f);
    return (uint32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

/* ---- public API ---- */

void ls_snap_init(LSSnapFile *snap, uint16_t w, uint16_t h,
                  uint16_t fps, LSModeFlag mode) {
    memset(snap, 0, sizeof(*snap));
    snap->version   = LS_LSNAP_VERSION;
    snap->width     = w;
    snap->height    = h;
    snap->fps       = fps;
    snap->mode_flag = mode;
}

void ls_snap_free(LSSnapFile *snap) {
    if (snap->frames) {
        for (uint32_t i = 0; i < snap->frame_count; i++)
            free(snap->frames[i].dirty_pixels);
        free(snap->frames);
        snap->frames = NULL;
    }
    snap->frame_count  = 0;
    snap->palette_size = 0;
}

int ls_snap_palette_find(const LSSnapFile *snap, uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < snap->palette_size; i++) {
        if (snap->palette[i][0] == r &&
            snap->palette[i][1] == g &&
            snap->palette[i][2] == b)
            return (int)i;
    }
    return -1;
}

static int palette_add(LSSnapFile *snap, uint8_t r, uint8_t g, uint8_t b) {
    int idx = ls_snap_palette_find(snap, r, g, b);
    if (idx >= 0) return idx;
    if (snap->palette_size >= LS_LSNAP_MAX_PALETTE) return -1;
    idx = snap->palette_size;
    snap->palette[idx][0] = r;
    snap->palette[idx][1] = g;
    snap->palette[idx][2] = b;
    snap->palette_size++;
    return idx;
}

int ls_snap_build_palette(LSSnapFile *snap, const LSAnimation *anim) {
    snap->palette_size = 0;
    for (uint16_t f = 0; f < anim->frame_count; f++) {
        const LSGrid *g = &anim->frames[f];
        size_t total = (size_t)g->width * g->height;
        for (size_t i = 0; i < total; i++) {
            LSPixel p = g->pixels[i];
            if (p.a == 0) continue;
            if (palette_add(snap, p.r, p.g, p.b) < 0)
                return -1;
        }
    }
    return 0;
}

uint32_t ls_snap_diff_frame(const LSGrid *prev, const LSGrid *cur,
                            LSDirtyPixel *out, uint32_t out_capacity,
                            const LSSnapFile *snap) {
    uint32_t count = 0;
    size_t total = (size_t)cur->width * cur->height;

    for (size_t i = 0; i < total && count < out_capacity; i++) {
        LSPixel cp = cur->pixels[i];
        int differs = 0;

        if (prev) {
            LSPixel pp = prev->pixels[i];
            differs = !ls_pixel_eq(cp, pp);
        } else {
            differs = (cp.a > 0);
        }

        if (differs) {
            int idx = ls_snap_palette_find(snap, cp.r, cp.g, cp.b);
            if (idx < 0) idx = 0;
            out[count].x = (uint16_t)(i % cur->width);
            out[count].y = (uint16_t)(i / cur->width);
            out[count].palette_idx = (uint8_t)idx;
            count++;
        }
    }
    return count;
}

int ls_snap_write(const char *path, const LSSnapFile *snap) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    /* header */
    fwrite(LS_LSNAP_MAGIC, 1, 4, f);
    write_u16(f, snap->version);
    write_u16(f, snap->width);
    write_u16(f, snap->height);
    write_u16(f, snap->fps);
    write_u16(f, (uint16_t)snap->mode_flag);
    write_u16(f, snap->palette_size);

    /* palette */
    for (uint16_t i = 0; i < snap->palette_size; i++)
        fwrite(snap->palette[i], 1, 3, f);

    /* frames */
    write_u32(f, snap->frame_count);
    for (uint32_t i = 0; i < snap->frame_count; i++) {
        const LSSnapFrame *fr = &snap->frames[i];
        write_u32(f, fr->dirty_count);
        for (uint32_t d = 0; d < fr->dirty_count; d++) {
            write_u16(f, fr->dirty_pixels[d].x);
            write_u16(f, fr->dirty_pixels[d].y);
            uint8_t pidx = fr->dirty_pixels[d].palette_idx;
            fwrite(&pidx, 1, 1, f);
        }
    }

    fclose(f);
    return 0;
}

int ls_snap_read(const char *path, LSSnapFile *snap) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    /* magic */
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, LS_LSNAP_MAGIC, 4) != 0) {
        fclose(f);
        return -1;
    }

    memset(snap, 0, sizeof(*snap));
    snap->version   = read_u16(f);
    snap->width     = read_u16(f);
    snap->height    = read_u16(f);
    snap->fps       = read_u16(f);
    snap->mode_flag = (LSModeFlag)read_u16(f);
    snap->palette_size = read_u16(f);

    if (snap->palette_size > LS_LSNAP_MAX_PALETTE) {
        fclose(f);
        return -1;
    }

    /* palette */
    for (uint16_t i = 0; i < snap->palette_size; i++)
        fread(snap->palette[i], 1, 3, f);

    /* frames */
    snap->frame_count = read_u32(f);
    snap->frames = (LSSnapFrame *)calloc(snap->frame_count, sizeof(LSSnapFrame));
    if (!snap->frames) { fclose(f); return -1; }

    for (uint32_t i = 0; i < snap->frame_count; i++) {
        LSSnapFrame *fr = &snap->frames[i];
        fr->dirty_count = read_u32(f);
        fr->dirty_pixels = (LSDirtyPixel *)malloc(fr->dirty_count * sizeof(LSDirtyPixel));
        if (!fr->dirty_pixels) { fclose(f); ls_snap_free(snap); return -1; }
        for (uint32_t d = 0; d < fr->dirty_count; d++) {
            fr->dirty_pixels[d].x = read_u16(f);
            fr->dirty_pixels[d].y = read_u16(f);
            fread(&fr->dirty_pixels[d].palette_idx, 1, 1, f);
        }
    }

    fclose(f);
    return 0;
}
