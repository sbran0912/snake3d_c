#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "lib3d.h"
#include "lib3d_util.h"
#include "grid.h"
#include "snake.h"

// snake3d - Grundgeruest: Bodengitter plus ein paar Wuerfel.
//
// Ein Bodengitter aus 40x40 Feldern. In der ersten Zeile (row 0)
// stehen 5 Wuerfel, die je genau ein Feld bedecken.
//
// Konvention: col zeigt in X-Richtung, row in Z-Richtung. Das Gitter
// liegt zentriert um den Ursprung in der XZ-Ebene (y=0).

#define EDGE_CELLS  300                            // Felder pro Achse
#define CELL_SIZE   0.5                           // Kantenlaenge eines Feldes
#define EDGE_LENGTH (EDGE_CELLS * CELL_SIZE)      // Kantenlaenge des Gitters
#define CUBE_SIZE   2.0                           // Wuerfelkante
#define FOOD_COUNT  5                             // Anzahl Futter-Cubes

// --- Globale Szenen-/Loop-Daten ---
static Scene scene;
static Mesh cubeMesh;
static Mesh gridMesh;
static ShapeStyle gridStyle;
static ShapeStyle cubeStyle;
static Instance *instances    = NULL; // arr_* von Instance (Schlange)
static Instance *food_instances = NULL; // arr_* von Instance (Futter)

// Snake
static Snake snake;

// Futter: 3 zufällige Gitterzellen
static Cell food_cells[FOOD_COUNT];

// Gitter-Parameter (zentriert um den Ursprung in der XZ-Ebene).
static Grid grid = { .edgeCells = EDGE_CELLS, .cellSize = CELL_SIZE };

// Loop (Ersatz fuer die draw-Closure in Go).
static void draw(void) {

    // Pfeiltasten abfragen und im Terminal ausgeben (je Tastendruck einmal).
    if (l3d_key_pressed(KEY_UP)){
        if (snake.directionY != 1) {
            snake.directionX = 0;
            snake.directionY = -1;
            printf("Nach oben\n");
        }
    }
    if (l3d_key_pressed(KEY_DOWN)){
        if (snake.directionY != -1) {
            snake.directionX = 0;
            snake.directionY = 1;
            printf("Nach unten\n");
        }
    }
    if (l3d_key_pressed(KEY_LEFT)) {
        if (snake.directionX != 1) {
            snake.directionX = -1;
            snake.directionY = 0;
            printf("Nach links\n");
        }
    }
    if (l3d_key_pressed(KEY_RIGHT)) {
        if (snake.directionX != -1) {
            snake.directionX = 1;
            snake.directionY = 0;
            printf("Nach rechts\n");
        }
    }

    move_snake(&snake, EDGE_CELLS);
    //printf("Snake head: row=%d, col=%d\n",snake.segments[snake.head].row,snake.segments[snake.head].col);

    int food_hit = check_food_hit(&snake, food_cells, FOOD_COUNT);
    if (food_hit >= 0) {
        printf("Futter %d getroffen! (row=%d, col=%d)\n",
               food_hit, food_cells[food_hit].row, food_cells[food_hit].col);
        grow_snake(&snake, EDGE_CELLS);
    }

    l3d_begin_scene(scene);

    // Bodengitter einmal
    //l3d_draw_mesh(gridMesh, gridStyle, NULL);

    // Alle Segmente als Instanz-Würfel aufbauen und zeichnen (blau)
    arr_delete(instances);
    instances = grid_push_cubes(instances, &grid, snake.segments,
                                arr_len(snake.segments), CUBE_SIZE,
                                parse_hex("#87CEEB"));

    l3d_draw_mesh_instanced(cubeMesh, cubeStyle,
                           (const Instance *) instances,
                           (int) arr_len(instances));

    // Futter-Cubes als rote Instances aufbauen und zeichnen
    arr_delete(food_instances);
    food_instances = grid_push_cubes(food_instances, &grid, food_cells,
                                     FOOD_COUNT, CUBE_SIZE,
                                     parse_hex("#FF0000"));

    l3d_draw_mesh_instanced(cubeMesh, cubeStyle,
                           (const Instance *) food_instances,
                           (int) arr_len(food_instances));
}

int main(void) {
    scene = scene_new();
    scene.cam.pos    = vec3(0, 50, 100);
    scene.cam.target = vec3(0, 0, 0);
    scene.cam.up     = vec3(0, 1, 1);
    scene.cam.far    = 500;

    // Meshes EINMAL erzeugen und wiederverwenden.
    cubeMesh = make_cube_mesh(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    gridMesh = make_grid_mesh(EDGE_LENGTH, EDGE_CELLS);

    gridStyle.stroke    = color_hex("#777774");
    gridStyle.lineWidth = 1;

    cubeStyle.stroke    = color_hex("#6b5200");
    cubeStyle.fill      = color_hex("#ffffff"); // wird pro Instanz ueberschrieben
    cubeStyle.lineWidth = 1;
    cubeStyle.mode      = STYLE_BOTH;

    snake = init_snake(100, 0, 0, 1, 1);
    instances = arr_create(10, sizeof(Instance));
    food_instances = arr_create(FOOD_COUNT, sizeof(Instance));

    // 3 zufällige Futter-Positionen generieren (ohne Duplikate)
    generate_food_cells(food_cells, FOOD_COUNT, EDGE_CELLS);

    int ok = l3d_init();
    if (!ok) {
        fprintf(stderr, "snake3d: Initialisierung fehlgeschlagen\n");
        return 1;
    }

    l3d_set_batch_sort(1);
    l3d_start_animation(draw);

    // Aufraeumen (nach dem Schliessen des Fensters).
    arr_free(instances);
    arr_free(food_instances);
    mesh_free(&cubeMesh);
    mesh_free(&gridMesh);
    snake_free(&snake);

    return 0;
}
