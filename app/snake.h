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
void generate_food_cells(Cell *out, int count, int edge_cells);

// Gibt Index des getroffenen Futters (0..count-1) oder -1 bei keinem Treffer.
int check_food_hit(const Snake *snake, const Cell *food_cells, int count);

#endif // SNAKE_H
