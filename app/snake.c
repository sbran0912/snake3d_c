#include "snake.h"

#include "lib3d.h"

// init_snake initialisiert die Schlange mit einem dynamischen Array
// fuer die Segmente.
Snake init_snake(int x, int y, int directionX, int directionY) {
    Snake snake = {
        .segments   = arr_create(1, sizeof(Point)),
        .head       = 0,
        .tail       = 0,
        .directionX = directionX,
        .directionY = directionY,
    };

    Point start = { x, y };
    snake.segments = arr_push(snake.segments, &start);
    return snake;
}

// free_snake gibt den dynamischen Speicher der Segmente frei.
void snake_free(Snake *snake) {
    arr_free(snake->segments);
    snake->segments = NULL;
    snake->head = 0;
    snake->tail = 0;
}
