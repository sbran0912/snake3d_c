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
    int new_row = snake->segments[snake->head].row + snake->directionY;
    int new_col = snake->segments[snake->head].col + snake->directionX;

    if (new_row < 0) {new_row = edge_cells - 1;}
    if (new_row > (int)edge_cells-1) {new_row = 0;}
    if (new_col < 0) {new_col = edge_cells - 1;}
    if (new_col > (int)edge_cells-1) {new_col = 0;}

    Cell new_head = {
        .row = new_row,
        .col = new_col
    };

    for (size_t i = snake->tail; i < snake->head; i++) {
        snake->segments[i] = snake->segments[i+1];
    }

    snake->segments[snake->head] = new_head;
}

void grow_snake(Snake* snake, size_t edge_cells) {
    int new_row = snake->segments[snake->head].row + snake->directionY;
    int new_col = snake->segments[snake->head].col + snake->directionX;

    if (new_row < 0) {new_row = edge_cells - 1;}
    if (new_row > (int)edge_cells-1) {new_row = 0;}
    if (new_col < 0) {new_col = edge_cells - 1;}
    if (new_col > (int)edge_cells-1) {new_col = 0;}

    Cell new_head = {
        .row = new_row,
        .col = new_col
    };
    snake->segments = arr_push(snake->segments, &new_head);
    snake->head++;

    for (size_t i = snake->tail; i < snake->head; i++) {
        printf("Segment %zu: row=%d, col=%d\n", i, snake->segments[i].row, snake->segments[i].col);
    }
}

// free_snake gibt den dynamischen Speicher der Segmente frei.
void snake_free(Snake *snake) {
    arr_free(snake->segments);
    snake->segments = NULL;
    snake->head = 0;
    snake->tail = 0;
}

// generate_food_cells erzeugt count zufällige, eindeutige Gitterzellen
// innerhalb von edge_cells x edge_cells und schreibt sie in out.
void generate_food_cells(Cell *out, int count, int edge_cells) {
    int placed = 0;
    while (placed < count) {
        Cell c = { .row = l3d_random(0, edge_cells), .col = l3d_random(0, edge_cells) };
        int doppelt = 0;
        for (int j = 0; j < placed; j++) {
            if (out[j].row == c.row && out[j].col == c.col) {
                doppelt = 1;
                break;
            }
        }
        if (!doppelt) {
            out[placed] = c;
            placed++;
        }
    }
}

// check_food_hit prüft, ob der Schlangenkopf einer Futterposition nahekommt.
// Gibt den Index des getroffenen Futters (0 .. count-1) zurück, sonst -1.
// Toleranz: +-2 Zellen in row und col.
int check_food_hit(const Snake *snake, const Cell *food_cells, int count) {
    Cell head = snake->segments[snake->head];
    for (int i = 0; i < count; i++) {
        int d_row = head.row - food_cells[i].row;
        int d_col = head.col - food_cells[i].col;
        if (d_row < 0) d_row = -d_row;
        if (d_col < 0) d_col = -d_col;
        if (d_row <= 2 && d_col <= 2) {
            return i;
        }
    }
    return -1;
}
