#include <stdio.h>

#include "lib3d.h"
#include "snake.h"

/* ====================================================================
 * snake3d - Grundgeruest: Bodengitter plus ein paar Wuerfel.
 *
 * Ein Bodengitter aus 40x40 Feldern. In der ersten Zeile (row 0)
 * stehen 5 Wuerfel, die je genau ein Feld bedecken.
 *
 * Konvention: col zeigt in X-Richtung, row in Z-Richtung. Das Gitter
 * liegt zentriert um den Ursprung in der XZ-Ebene (y=0).
 * ================================================================== */

#define EDGE_CELLS  40                            // Felder pro Achse
#define CELL_SIZE   1.0                           // Kantenlaenge eines Feldes
#define EDGE_LENGTH (EDGE_CELLS * CELL_SIZE)      // Kantenlaenge des Gitters
#define CUBE_SIZE   0.9                           // Wuerfelkante
#define CUBE_COUNT  5                             // Wuerfel in der ersten Zeile

/* --- Globale Szenen-/Loop-Daten --- */
static Scene      g_scene;
static Mesh       g_cubeMesh;
static Mesh       g_gridMesh;
static ShapeStyle g_gridStyle;
static ShapeStyle g_cubeStyle;
static void      *instances    = NULL; // arr_* von Instance

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

// append_cube haengt eine Wuerfel-Instanz an der Feldmitte (x, z) an.
static void *append_cube(void *arr, double x, double z, ColorState color) {
    Instance in;
    in.model = mat4_translate(x, CUBE_SIZE / 2.0, z);
    in.color = color;
    return arr_push(arr, &in);
}

// build_instances legt die Wuerfel der ersten Zeile (row 0) an.
static void *build_instances(void *arr, ColorState color) {
    for (int c = 0; c < CUBE_COUNT; c++) {
        CellPos p = cellCenter(0, c);
        arr = append_cube(arr, p.x, p.z, color);
    }
    return arr;
}

// Loop (Ersatz fuer die draw-Closure in Go).
static void draw(void) {
    l3d_begin_scene(g_scene);

    // Bodengitter (XZ-Ebene bei y=0) aus dem geteilten Mesh.
    l3d_draw_mesh(g_gridMesh, g_gridStyle, NULL);

    // Die Wuerfel der ersten Zeile in EINEM Instancing-Draw-Call.
    l3d_draw_mesh_instanced(g_cubeMesh, g_cubeStyle,
                           (const Instance *) instances,
                           (int) arr_len(instances));
}

int main(void) {
    g_scene = scene_new();
    g_scene.cam.pos    = vec3(17, 24, 32);
    g_scene.cam.target = vec3(0, 0, 0);
    g_scene.cam.up     = vec3(0, 1, 0);
    g_scene.cam.far    = 500;

    // Meshes EINMAL erzeugen und wiederverwenden.
    g_cubeMesh = make_cube_mesh(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    g_gridMesh = make_grid_mesh(EDGE_LENGTH, EDGE_CELLS);

    g_gridStyle.stroke    = color_hex("#777774");
    g_gridStyle.lineWidth = 1;

    g_cubeStyle.stroke    = color_hex("#6b5200");
    g_cubeStyle.fill      = color_hex("#ffffff"); // wird pro Instanz ueberschrieben
    g_cubeStyle.lineWidth = 1;
    g_cubeStyle.mode      = STYLE_BOTH;

    instances = arr_create((size_t) CUBE_COUNT, sizeof(Instance));
    instances = build_instances(instances, parse_hex("#87CEEB"));

    if (!l3d_init()) {
        fprintf(stderr, "snake3d: Initialisierung fehlgeschlagen\n");
        return 1;
    }

    l3d_set_batch_sort(1);
    l3d_start_animation(draw);

    // Aufraeumen (nach dem Schliessen des Fensters).
    arr_free(instances);
    mesh_free(&g_cubeMesh);
    mesh_free(&g_gridMesh);

    return 0;
}
