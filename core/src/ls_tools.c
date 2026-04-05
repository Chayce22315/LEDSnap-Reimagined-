#include "ls_tools.h"

void ls_tool_pen(LSGrid *grid, uint16_t x, uint16_t y, LSPixel color) {
    if (!ls_grid_in_bounds(grid, x, y)) return;
    *ls_grid_at(grid, x, y) = color;
}

void ls_tool_eraser(LSGrid *grid, uint16_t x, uint16_t y) {
    if (!ls_grid_in_bounds(grid, x, y)) return;
    *ls_grid_at(grid, x, y) = LS_PIXEL_TRANSPARENT;
}

/*
 * Iterative scanline flood-fill.
 * Uses an explicit stack to avoid deep recursion on large grids.
 */

typedef struct { int16_t x, y; } FillPoint;

void ls_tool_bucket(LSGrid *grid, uint16_t sx, uint16_t sy, LSPixel fill_color) {
    if (!ls_grid_in_bounds(grid, sx, sy)) return;

    LSPixel target = *ls_grid_at(grid, sx, sy);
    if (ls_pixel_eq(target, fill_color)) return;

    size_t capacity = (size_t)grid->width * grid->height;
    FillPoint *stack = (FillPoint *)malloc(capacity * sizeof(FillPoint));
    if (!stack) return;

    size_t top = 0;
    stack[top++] = (FillPoint){ (int16_t)sx, (int16_t)sy };

    while (top > 0) {
        FillPoint p = stack[--top];
        int x = p.x;
        int y = p.y;

        if (!ls_grid_in_bounds(grid, x, y)) continue;

        LSPixel *px = ls_grid_at(grid, (uint16_t)x, (uint16_t)y);
        if (!ls_pixel_eq(*px, target)) continue;

        /* scan left */
        int left = x;
        while (left > 0 && ls_pixel_eq(*ls_grid_at(grid, (uint16_t)(left - 1), (uint16_t)y), target))
            left--;

        /* scan right */
        int right = x;
        while (right < grid->width - 1 && ls_pixel_eq(*ls_grid_at(grid, (uint16_t)(right + 1), (uint16_t)y), target))
            right++;

        /* fill the scanline and push neighbors above/below */
        for (int fx = left; fx <= right; fx++) {
            *ls_grid_at(grid, (uint16_t)fx, (uint16_t)y) = fill_color;

            if (y > 0 && ls_pixel_eq(*ls_grid_at(grid, (uint16_t)fx, (uint16_t)(y - 1)), target)) {
                if (top < capacity)
                    stack[top++] = (FillPoint){ (int16_t)fx, (int16_t)(y - 1) };
            }
            if (y < grid->height - 1 && ls_pixel_eq(*ls_grid_at(grid, (uint16_t)fx, (uint16_t)(y + 1)), target)) {
                if (top < capacity)
                    stack[top++] = (FillPoint){ (int16_t)fx, (int16_t)(y + 1) };
            }
        }
    }

    free(stack);
}
