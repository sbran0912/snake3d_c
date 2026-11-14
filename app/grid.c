#include "grid.h"

#include "lib3d.h"

CellPos grid_cell_center(const Grid *g, int row, int col) {
    double half = ((double) g->edgeCells * g->cellSize) / 2.0;
    CellPos p;
    p.x = -half + ((double) col + 0.5) * g->cellSize;
    p.z = -half + ((double) row + 0.5) * g->cellSize;
    return p;
}

void *grid_push_cubes(void *arr, const Grid *g, const Cell *cells, size_t count,
                      double height, ColorState color) {
    for (size_t i = 0; i < count; i++) {
        CellPos p = grid_cell_center(g, cells[i].row, cells[i].col);
        Instance in = {
            .model = mat4_translate(p.x, height / 2.0, p.z),
            .color = color,
        };
        arr = arr_push(arr, &in);
    }
    return arr;
}
