#ifndef LIB3D_H
#define LIB3D_H

// lib3d - C-Port des Go-Pakets lib3d.
//
// Inhalt (vollstaendig uebertragen aus dem Go-Original):
//   mesh.go / body.go / mesh_io.go          -> Mesh-Erzeugung, Bodies,
//                                              OBJ-Loader
//   draw.go / shader.go / renderer.go       -> Zeichen-Fassade, Renderer
//                                              (OpenGL 3.3 Core, GLFW)
//
// Die Basis-Helfer (vec.go / mat4.go / color.go / math.go sowie die
// generischen Container arr_* und hm_*) liegen in lib3d_util.h und
// werden hier eingebunden.
//
// Konventionen (identisch zum Go-Code):
//   - +Y zeigt nach oben, Kamera blickt in +Z.
//   - Matrix4x4 ist zeilen-major (m[zeile][spalte]) vom Typ double;
//     fuer OpenGL wird beim Hochladen in Spalten-major Float32 gewandelt.
//   - Mesh-Daten sind float-Tripel (xyz), als dynamisch allozierte
//     Arrays; mesh_free() gibt sie wieder frei.
//   - Der Renderer arbeitet mit FIXEN Puffern (keine dynamischen
//     Datenstrukturen) - Kapazitaeten siehe MAX_*-Defines in lib3d.c.

#include <stddef.h>
#include <stdint.h>

#include "lib3d_util.h"

#ifdef __cplusplus
extern "C" {
#endif

// TYPEN
//
// Vec3, Vec2, Mat4x4, ColorState und ColorInput kommen aus
// lib3d_util.h (oben eingebunden).
typedef enum { STYLE_STROKE = 0, STYLE_FILL = 1, STYLE_BOTH = 2 } DrawStyle;
typedef enum { EFFECT_FLAT = 0, EFFECT_GRADIENT = 1, EFFECT_PULSE = 2 } EffectMode;

// Mesh: reine Zeichen-Geometrie (lokale Koordinaten).
typedef struct {
    float *triangles;   // xyz-Tripel einer Dreiecksliste (oder NULL)
    float *lines;       // xyz-Tripel einer Linienliste (oder NULL)
    int    tri_floats;  // Anzahl Floats in triangles
    int    line_floats; // Anzahl Floats in lines
} Mesh;

// Face: Dreieck eines Meshes samt (normierter) Flaechennormale.
typedef struct {
    Vec3 a, b, c, normal;
} Face;

typedef struct {
    int     isHit;
    double  t;          // Parameter entlang der Strecke, 0..1
    Vec3    point;
    Vec3    normal;
} RayHit;

typedef struct {
    Vec3   point;
    Vec3   normal;
    double distance;       // ungerichteter Abstand
    double signedDistance; // >0 aussen, <0 innen (bzgl. Normalen)
} ClosestFaceHit;

// Body: platzierbarer, kollisionsfaehiger Koerper.
typedef struct {
    Mesh   geometry; // Zeichengeometrie (lokal) - Eigentum des Aufrufers
    Face  *faces;    // lokale Faces (inkl. Normalen) - von make_body()
    int    face_count;
    Vec3   pos;
    Vec3   rot;      // Euler-Winkel in Radiant (NULL-Vektor = keine Rotation)
} Body;

// Eine Instanz fuer GPU-Instancing: Modell-Matrix + Farbe.
typedef struct {
    Mat4x4     model;
    ColorState color;
} Instance;

// Zeichen-Optionen (Immediate-Mode). set == 0 -> nicht ueberschreiben.
typedef struct {
    ColorInput stroke;
    ColorInput fill;
    double     lineWidth;
    double     pointSize;
} PrimitiveStyle;

// Zeichen-Optionen fuer Meshes/Primitive inkl. Modus.
typedef struct {
    ColorInput stroke;
    ColorInput fill;
    double     lineWidth;
    double     pointSize;
    DrawStyle  mode;
    double     z;
    int        segments;
} ShapeStyle;

typedef struct { Vec3 pos, target, up; double fovY, near, far; } Camera;
typedef struct { int enabled; double near, far; ColorState color; } Fog;
typedef struct { Camera cam; Vec3 sunDir; Fog fog; ColorState background; } Scene;

// MESH  (Port von mesh.go)
Mesh make_cube_mesh(double w, double h, double d);
Mesh make_pyramid_mesh(double base, double height);
Mesh make_sphere_mesh(double radius, int slices, int stacks);
Mesh make_grid_mesh(double size, int cells);
Mesh scale_mesh(Mesh mesh, double s);        // liefert neue Kopie
void mesh_free(Mesh *m);

// MESH-LADEN  (Port von mesh_io.go, Wavefront-OBJ)
#define L3D_OBJ_OK         0
#define L3D_OBJ_ERR_FILE   (-1)
#define L3D_OBJ_ERR_VERTEX (-2)
#define L3D_OBJ_ERR_INDEX  (-3)
int load_obj(const char *path, Mesh *out);   // 0 = Erfolg, sonst Fehlercode

// BODY / FACES  (Port von body.go)
Face *build_faces(const Mesh *mesh, int *out_count); // malloc; faces_free()
void  faces_free(Face *faces);
Face  transform_face(Face f, const Mat4x4 *model);
Face *transform_faces(const Face *faces, int count, const Mat4x4 *model,
                     int *out_count);
RayHit raycast_segment(Vec3 p1, Vec3 p2, const Face *faces, int count);
ClosestFaceHit closest_point_to_face(Vec3 p, Face face);

Body make_body(Mesh geometry, Vec3 pos, Vec3 rot); // rot = {0,0,0} fuer keine
Mat4x4 body_model_matrix(const Body *b);
Face *body_world_faces(const Body *b, int *out_count);
void  body_free(Body *b);                    // gibt nur faces frei

// KAMERA / SZENE
Mat4x4 camera_view(Camera c);
Mat4x4 camera_projection(Camera c, double aspect);
Scene  scene_new(void);

// TASTEN-CODES  (exakt die GLFW_KEY_*-Codes, Port aus renderer.go)
#define KEY_SPACE        32
#define KEY_APOSTROPHE   39
#define KEY_COMMA        44
#define KEY_MINUS        45
#define KEY_PERIOD       46
#define KEY_SLASH        47
#define KEY_0            48
#define KEY_1            49
#define KEY_2            50
#define KEY_3            51
#define KEY_4            52
#define KEY_5            53
#define KEY_6            54
#define KEY_7            55
#define KEY_8            56
#define KEY_9            57
#define KEY_SEMICOLON    59
#define KEY_EQUAL        61
#define KEY_A            65
#define KEY_B            66
#define KEY_C            67
#define KEY_D            68
#define KEY_E            69
#define KEY_F            70
#define KEY_G            71
#define KEY_H            72
#define KEY_I            73
#define KEY_J            74
#define KEY_K            75
#define KEY_L            76
#define KEY_M            77
#define KEY_N            78
#define KEY_O            79
#define KEY_P            80
#define KEY_Q            81
#define KEY_R            82
#define KEY_S            83
#define KEY_T            84
#define KEY_U            85
#define KEY_V            86
#define KEY_W            87
#define KEY_X            88
#define KEY_Y            89
#define KEY_Z            90
#define KEY_LEFT_BRACKET 91
#define KEY_BACKSLASH    92
#define KEY_RIGHT_BRACKET 93
#define KEY_GRAVE_ACCENT 96
#define KEY_ESCAPE       256
#define KEY_ENTER        257
#define KEY_TAB          258
#define KEY_BACKSPACE    259
#define KEY_INSERT       260
#define KEY_DELETE       261
#define KEY_RIGHT        262
#define KEY_LEFT         263
#define KEY_DOWN         264
#define KEY_UP           265
#define KEY_PAGE_UP      266
#define KEY_PAGE_DOWN    267
#define KEY_HOME         268
#define KEY_END          269
#define KEY_CAPS_LOCK    280
#define KEY_F1           290
#define KEY_F2           291
#define KEY_F3           292
#define KEY_F4           293
#define KEY_F5           294
#define KEY_F6           295
#define KEY_F7           296
#define KEY_F8           297
#define KEY_F9           298
#define KEY_F10          299
#define KEY_F11          300
#define KEY_F12          301
#define KEY_LEFT_SHIFT   340
#define KEY_LEFT_CONTROL 341
#define KEY_LEFT_ALT     342
#define KEY_LEFT_SUPER   343
#define KEY_RIGHT_SHIFT  344
#define KEY_RIGHT_CONTROL 345
#define KEY_RIGHT_ALT    346
#define KEY_RIGHT_SUPER  347

// RENDER-API  (Port von renderer.go / draw.go / shader.go)

// Setup & Schleife
int  l3d_init(void);                         // 1 = ok, 0 = Fehler
void l3d_start_animation(void (*draw)(void));
void l3d_no_loop(void);

// Fenster / Eingabe
int  l3d_get_width(void);
int  l3d_get_height(void);
int  l3d_mouse_down(void);
int  l3d_mouse_up(void);                     // konsumierend
int  l3d_is_fullscreen(void);
void l3d_set_fullscreen(int on);
void l3d_toggle_fullscreen(void);
int  l3d_key_down(int key);
int  l3d_key_pressed(int key);                // konsumierend

// Matrizen & Szene
void l3d_set_projection(const Mat4x4 *m);
void l3d_set_view(const Mat4x4 *m);
void l3d_apply_camera(Camera cam);
void l3d_begin_scene(Scene scene);

// Batching
void l3d_flush_batch(void);
int  l3d_pending_draw_calls(void);
int  l3d_pending_instanced_draw_calls(void);
void l3d_set_batch_sort(int on);
void l3d_compact_batch(void);

// Uniforms
void l3d_set_gradient_center(double cx, double cy, double cz, double radius);
void l3d_set_fog(double near, double far, double r, double g, double b, double a);
void l3d_set_light_direction(double x, double y, double z);

// Zeichenzustand
void l3d_fill_color(ColorState color);
void l3d_fill_color_input(ColorInput color);
void l3d_stroke_color(ColorState color);
void l3d_stroke_color_input(ColorInput color);
void l3d_stroke_width(double w);
void l3d_set_effect(EffectMode effect);
void l3d_set_gradient(ColorState color);
void l3d_set_gradient_input(ColorInput color);
void l3d_background(ColorState color);
void l3d_background_input(ColorInput color);
void l3d_point_size(double px);

// Zeichen-Fassade (draw.go)
void l3d_draw_mesh(Mesh mesh, ShapeStyle style, const Mat4x4 *model);
void l3d_draw_mesh_instanced(Mesh mesh, ShapeStyle style,
                             const Instance *instances, int count);
void l3d_draw_point(double x, double y, double z, PrimitiveStyle style,
                    const Mat4x4 *model);
void l3d_draw_line(double x1, double y1, double z1, double x2, double y2, double z2,
                   PrimitiveStyle style, const Mat4x4 *model);
void l3d_draw_triangle(double x1, double y1, double z1, double x2, double y2,
                       double z2, double x3, double y3, double z3,
                       ShapeStyle style, const Mat4x4 *model);
void l3d_draw_shape(double x1, double y1, double z1, double x2, double y2,
                    double z2, double x3, double y3, double z3, double x4,
                    double y4, double z4, ShapeStyle style, const Mat4x4 *model);
void l3d_draw_rect(double x, double y, double w, double h, ShapeStyle style,
                   const Mat4x4 *model);
void l3d_draw_circle(double x, double y, double z, double radius, ShapeStyle style,
                     const Mat4x4 *model);
void l3d_draw_polygon(const double *pts, int nfloats, ShapeStyle style,
                      const Mat4x4 *model);

// Immediate-Primitive (renderer.go)
void l3d_point(double x, double y, double z, const Mat4x4 *model);
void l3d_line(double x1, double y1, double z1, double x2, double y2, double z2,
              const Mat4x4 *model);
void l3d_triangle(double x1, double y1, double z1, double x2, double y2,
                  double z2, double x3, double y3, double z3, DrawStyle style,
                  const Mat4x4 *model);
void l3d_shape(double x1, double y1, double z1, double x2, double y2, double z2,
               double x3, double y3, double z3, double x4, double y4, double z4,
               DrawStyle style, const Mat4x4 *model);
void l3d_rect(double x, double y, double w, double h, DrawStyle style, double z,
              const Mat4x4 *model);
void l3d_circle(double x, double y, double z, double radius, DrawStyle style,
                int segments, const Mat4x4 *model);
void l3d_polygon(const double *pts, int nfloats, DrawStyle style,
                 const Mat4x4 *model);

#ifdef __cplusplus
}
#endif

#endif // LIB3D_H
