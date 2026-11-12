#include <stddef.h>
#include <stdio.h>

#include "lib3d.h"
#include "lib3d_util.h"
#include "snake.h"

// snake3d - Grundgeruest: Bodengitter plus ein paar Wuerfel.
//
// Ein Bodengitter aus 40x40 Feldern. In der ersten Zeile (row 0)
// stehen 5 Wuerfel, die je genau ein Feld bedecken.
//
// Konvention: col zeigt in X-Richtung, row in Z-Richtung. Das Gitter
// liegt zentriert um den Ursprung in der XZ-Ebene (y=0).

#define EDGE_CELLS  40                            // Felder pro Achse
#define CELL_SIZE   1.0                           // Kantenlaenge eines Feldes
#define EDGE_LENGTH (EDGE_CELLS * CELL_SIZE)      // Kantenlaenge des Gitters
#define CUBE_SIZE   0.9                           // Wuerfelkante
#define SNAKE_COUNT 200

// --- Globale Szenen-/Loop-Daten ---
static Scene scene;
static Mesh cubeMesh;
static Mesh gridMesh;
static ShapeStyle gridStyle;
static ShapeStyle cubeStyle;
static Instance *instances    = NULL; // arr_* von Instance

// Snake
static Snake snake;

// Weltkoordinaten der Feldmitte in der XZ-Ebene.
typedef struct { double x, z; } CellPos;

// cellCenter rechnet Weltkoordinaten der Feldmitte fuer ein Feld
// (row, col). col -> X-Achse, row -> Z-Achse, Feldmitte +0.5-Verschiebung.
static CellPos cellCenter(int row, int col) {
    double half = EDGE_LENGTH / 2.0;
    CellPos p;
    p.x = -half + ((double) col + 0.5) * CELL_SIZE;
    p.z = -half + ((double) row + 0.5) * CELL_SIZE;
    return p;
}


// build_instances haengt fuer jedes Segment eine Wuerfel-Instanz an der
// Feldmitte (x, z) an.
static void *build_instances(void *arr, ColorState color, const Point *segments, size_t count) {
    for (size_t i = 0; i < count; i++) {
        CellPos p = cellCenter(segments[i].y, segments[i].x);
        Instance in;
        in.model = mat4_translate(p.x, CUBE_SIZE / 2.0, p.z);
        in.color = color;
        arr = arr_push(arr, &in);
    }
    return arr;
}

// Loop (Ersatz fuer die draw-Closure in Go).
static void draw(void) {
    l3d_begin_scene(scene);

    // Bodengitter (XZ-Ebene bei y=0) aus dem geteilten Mesh.
    l3d_draw_mesh(gridMesh, gridStyle, NULL);

    // Die Wuerfel der ersten Zeile in EINEM Instancing-Draw-Call.
    l3d_draw_mesh_instanced(cubeMesh, cubeStyle,
                           (const Instance *) instances,
                           (int) arr_len(instances));
}

int main(void) {
    scene = scene_new();
    scene.cam.pos    = vec3(17, 24, 32);
    scene.cam.target = vec3(0, 0, 0);
    scene.cam.up     = vec3(0, 1, 0);
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

    snake = init_snake(10, 10, 1, 1);
    instances = arr_create(SNAKE_COUNT, sizeof(Instance));
    instances = build_instances(instances, parse_hex("#87CEEB"), snake.segments, arr_len(snake.segments));

    if (!l3d_init()) {
        fprintf(stderr, "snake3d: Initialisierung fehlgeschlagen\n");
        return 1;
    }

    l3d_set_batch_sort(1);
    l3d_start_animation(draw);

    // Aufraeumen (nach dem Schliessen des Fensters).
    arr_free(instances);
    mesh_free(&cubeMesh);
    mesh_free(&gridMesh);
    snake_free(&snake);

    return 0;
}
