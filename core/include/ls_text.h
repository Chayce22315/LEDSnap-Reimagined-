#ifndef LS_TEXT_H
#define LS_TEXT_H

#include "ls_types.h"

#define LS_FONT_CHAR_W 5
#define LS_FONT_CHAR_H 7
#define LS_FONT_FIRST  32  /* space */
#define LS_FONT_LAST   126 /* tilde */
#define LS_FONT_COUNT  (LS_FONT_LAST - LS_FONT_FIRST + 1)

/* Built-in 5x7 bitmap font stored as 7 bytes per glyph (1 row per byte, MSB-left). */
extern const uint8_t ls_font_5x7[LS_FONT_COUNT][LS_FONT_CHAR_H];

typedef struct {
    const char *text;
    float       x_offset;     /* current scroll position in pixels (float for sub-pixel) */
    float       scroll_speed; /* pixels per second (negative = scroll left) */
    LSPixel     color;
    uint8_t     font_height;  /* grid rows (always 7 for built-in font) */
    uint16_t    y_position;   /* top row on grid */
    int         wrap;         /* 1 = seamless loop, 0 = stop at edge */
} LSTextElement;

void ls_text_init(LSTextElement *elem, const char *text,
                  float speed, LSPixel color, uint16_t y);

void ls_text_update(LSTextElement *elem, float delta_time, uint16_t grid_width);
void ls_text_render(const LSTextElement *elem, LSGrid *target);

#endif /* LS_TEXT_H */
