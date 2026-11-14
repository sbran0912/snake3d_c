#ifndef SNAKE_H
#define SNAKE_H

#include "grid.h"   // Cell

typedef struct {
    Cell  *segments; // dynamisches Array
    int    head;
    int    tail;
    int    directionX;
    int    directionY;
} Snake;

// init_snake initialisiert die Schlange mit einem dynamischen Array
// fuer die Segmente.
Snake init_snake(int x, int y, int directionX, int directionY);
void snake_free(Snake *snake);

#endif // SNAKE_H
