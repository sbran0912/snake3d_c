#include "snake.h"
#include "lib3d_util.h"
#include <stddef.h>
#include <stdio.h>

//#include "lib3d.h"

// init_snake initialisiert die Schlange mit einem dynamischen Array
// fuer die Segmente.
Snake init_snake(int x, int y, int directionX, int directionY, const size_t count) {
    Snake snake = {
        .segments   = arr_create(count, sizeof(Cell)),
        .head       = 0,
        .tail       = 0,
        .directionX = directionX,
        .directionY = directionY,
    };

    Cell start = { .row = y, .col = x };
    snake.segments = arr_push(snake.segments, &start);
    return snake;
}

void move_snake(Snake* snake, size_t edge_cells) {
    //  head erstes element und tail letztes element des array
    // zuerst in einer schleifen die position an de nachfolger
    // übergeben, danach head auf neue position setzen (new_head)

    int row = snake->segments[snake->head].row + snake->directionY;
    int col = snake->segments[snake->head].col + snake->directionX;

    if (row < 0) {row = edge_cells - 1;}
    if (row > (int)edge_cells-1) {row = 0;}
    if (col < 0) {col = edge_cells - 1;}
    if (col > (int)edge_cells-1) {col = 0;}

    Cell new_head = {
        .row = row,
        .col = col
    };

    printf("New head position: row=%d, col=%d\n", new_head.row, new_head.col);
    for (size_t i = snake->tail; i > snake->head; i--) {
        snake->segments[i] = snake->segments[i-1];
    }

    snake->segments[snake->head] = new_head;

}

void grow_snake(Snake* snake, size_t edge_cells) {
    int row = snake->segments[snake->head].row + snake->directionY;
    int col = snake->segments[snake->head].col + snake->directionX;

    if (row < 0) {row = edge_cells - 1;}
    if (row > (int)edge_cells-1) {row = 0;}
    if (col < 0) {col = edge_cells - 1;}
    if (col > (int)edge_cells-1) {col = 0;}

    Cell new_head = {
        .row = row,
        .col = col
    };

    Cell* tmp = arr_create(20, sizeof(Cell));
    tmp = arr_push(tmp, &new_head);
    for (size_t i = 0; i <= snake->tail; i++) {
        tmp = arr_push(tmp, &snake->segments[i]);
    }
    snake->tail++;
    arr_free(snake->segments);
    snake->segments = tmp;
}

// free_snake gibt den dynamischen Speicher der Segmente frei.
void snake_free(Snake *snake) {
    arr_free(snake->segments);
    snake->segments = NULL;
    snake->head = 0;
    snake->tail = 0;
}
