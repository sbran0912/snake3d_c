#ifndef SNAKE_H
#define SNAKE_H

#include "grid.h"   // Cell
#include <stddef.h>

typedef struct {
    Cell  *segments; // dynamisches Array
    size_t    head;
    size_t    tail;
    int    directionX;
    int    directionY;
} Snake;

// init_snake initialisiert die Schlange mit einem dynamischen Array
// fuer die Segmente.
Snake init_snake(int x, int y, int directionX, int directionY, const size_t count);
void move_snake(Snake* snake, size_t edge_cells);
void grow_snake(Snake* snake, size_t edge_cells);
void snake_free(Snake *snake);

#endif // SNAKE_H
