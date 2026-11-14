#ifndef GRID_H
#define GRID_H

#include <stddef.h>

#include "lib3d_util.h"   // ColorState

// Gitter, zentriert um den Ursprung in der XZ-Ebene.
typedef struct {
    int    edgeCells;   // Felder pro Achse
    double cellSize;    // Kantenlaenge eines Feldes
} Grid;

// Gitterzelle: row -> Z-Achse, col -> X-Achse.
typedef struct {
    int row;
    int col;
} Cell;

// Weltkoordinaten der Feldmitte in der XZ-Ebene.
typedef struct { double x, z; } CellPos;

// grid_cell_center rechnet die Weltkoordinaten der Feldmitte fuer ein Feld
// (row, col). col -> X-Achse, row -> Z-Achse, Feldmitte +0.5-Verschiebung.
CellPos grid_cell_center(const Grid *g, int row, int col);

// grid_push_cubes haengt fuer jede Zelle eine Wuerfel-Instanz der Hoehe
// `height` an und liefert das (evtl. verschobene) arr_*-Array zurueck.
void *grid_push_cubes(void *arr, const Grid *g, const Cell *cells, size_t count, double height, ColorState color);

#endif // GRID_H
