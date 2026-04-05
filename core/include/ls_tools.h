#ifndef LS_TOOLS_H
#define LS_TOOLS_H

#include "ls_types.h"

void ls_tool_pen(LSGrid *grid, uint16_t x, uint16_t y, LSPixel color);
void ls_tool_eraser(LSGrid *grid, uint16_t x, uint16_t y);
void ls_tool_bucket(LSGrid *grid, uint16_t x, uint16_t y, LSPixel fill_color);

#endif /* LS_TOOLS_H */
