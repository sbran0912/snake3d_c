#ifndef SNAKE_H
#define SNAKE_H

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point *segments; // dynamisches Array
    int    head;
    int    tail;
    int    directionX;
    int    directionY;
} Snake;

// init_snake initialisiert die Schlange mit einem dynamischen Array
// fuer die Segmente.
Snake init_snake(int x, int y, int directionX, int directionY);

#endif // SNAKE_H
