#ifndef LIB3D_UTIL_H
#define LIB3D_UTIL_H

// lib3d_util - Basis-Helfer des C-Ports des Go-Pakets lib3d.
//
// Inhalt (Port von vec.go / mat4.go / color.go / math.go):
//   - Grundtypen Vec3, Vec2, Mat4x4, ColorState, ColorInput
//   - Vektor- und Matrix-Rechnung
//   - Farb-Parser und Farb-Eingaenge
//   - Mathe-Helfer (project, constrain, random, limit_num)
//   - dynamisches Array (arr_*)
//   - generische Hash-Map (hm_*)
//
// Konventionen (identisch zum Go-Code):
//   - +Y zeigt nach oben, Kamera blickt in +Z.
//   - Mat4x4 ist zeilen-major (m[zeile][spalte]) vom Typ double;
//     fuer OpenGL wird beim Hochladen in Spalten-major Float32 gewandelt.
//
// Implementierung: lib3d_util.c

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// TYPEN
typedef struct { double x, y, z; } Vec3;
typedef struct { double x, y, s; } Vec2;
typedef struct { double m[4][4]; } Mat4x4;   // zeilen-major

typedef struct { double r, g, b, a; } ColorState;
typedef struct { int set; ColorState color; } ColorInput;

// ALLGEMEINE HELPER-FUNKTIONEN
size_t SafeTextCopy(char *dst, size_t dst_size, const char *src);

// Vektoren 3D
Vec3 vec3(double x, double y, double z);           // NewVec3
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 a, double s);
Vec3 vec3_mag(Vec3 a, double m);                    // auf Ziel-Laenge skalieren
Vec3 vec3_limit(Vec3 a, double maxLen);
Vec3 vec3_negate(Vec3 a);
double vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
double vec3_squared_length(Vec3 a);
double vec3_length(Vec3 a);
double vec3_distance(Vec3 a, Vec3 b);
Vec3 vec3_normalize(Vec3 a);
Vec3 vec3_lerp(Vec3 a, Vec3 b, double t);
Vec3 vec3_clone(Vec3 a);
int  vec3_equals(Vec3 a, Vec3 b, double epsilon);
Vec3 vec3_transform(Vec3 v, const Mat4x4 *m);
Vec3 vec3_transform_dir(Vec3 v, const Mat4x4 *m);

// MATRIX  (Port von mat4.go)
Mat4x4 mat4_identity(void);
Mat4x4 mat4_translate(double dx, double dy, double dz);
Mat4x4 mat4_rotate(double ax, double ay, double az); // Rz * Ry * Rx
Mat4x4 mat4_mult(const Mat4x4 *a, const Mat4x4 *b);
Mat4x4 mat4_view_world(const Mat4x4 *view, const Mat4x4 *world);
Mat4x4 mat4_lookat(Vec3 cameraPos, Vec3 target, Vec3 up);
Vec3   world_to_camera(Vec3 point, const Mat4x4 *view, const Mat4x4 *world);
Vec3   rotate_around(Vec3 point, Vec3 pivot, const Mat4x4 *rotation);
Mat4x4 mat4_perspective(double fovY, double aspect, double near, double far);

// FARBE  (Port von color.go)
ColorState parse_hex(const char *hex);       // "#rgb" / "#rrggbb"
ColorState parse_gray(double gray);          // 0..255
ColorState parse_rgb(double r, double g, double b);    // 0..255
ColorState parse_rgba(double r, double g, double b, double a); // 0..255
int col_eq(ColorState a, ColorState b);

ColorInput color_hex(const char *hex);
ColorInput color_gray(double v);
ColorInput color_rgb(double r, double g, double b);
ColorInput color_rgba(double r, double g, double b, double a);
ColorInput color_of(ColorState c);

// MATH-HILFEN  (Port von math.go)
Vec2 project(double fov, Vec3 v);            // 2D-Projektion fuer Overlays
double l3d_constrain(double value, double lo, double hi);
int    l3d_random(int n1, int n2);            // ganzzahlig in [n1, n2)
double l3d_random_float(double lo, double hi);// in [lo, hi)
float  limit_num(float number, float limit);  // begrenzt den Betrag, Vorzeichen bleibt

//Dynamisches Array
void  *arr_create(size_t capacity, size_t elemsize);
size_t arr_len(void *arr);
size_t arr_cap(void *arr);
void  *arr_push(void *arr, void *val);
void   arr_pop(void *arr, size_t idx);
void  *arr_reduce(void *arr);
void   arr_delete(void *arr);  // setzt Laenge auf 0 (Kapazitaet bleibt)
void   arr_free(void *arr);

// HASH-MAP
typedef uint64_t (*HashMap_HashFn)(const void *key);
typedef int (*HashMap_EqFn)(const void *keyA, const void *keyB);

typedef struct HmBucket HmBucket;

typedef struct HashMap {
    HmBucket **buckets;
    size_t     nbuckets;
    size_t     count;
    size_t     keysize;
    size_t     valsize;
    HashMap_HashFn hash;
    HashMap_EqFn   eq;
} HashMap;

HashMap *hm_create(int capacity, size_t keysize, size_t valsize,
                   HashMap_HashFn hash, HashMap_EqFn eq);
void     hm_free(HashMap *m);
void     hm_clear(HashMap *m);
size_t   hm_size(HashMap *m);
int      hm_contains(HashMap *m, const void *key);
void    *hm_get(HashMap *m, const void *key);   // NULL, wenn abwesend
void     hm_put(HashMap *m, const void *key, const void *val);
void     hm_remove(HashMap *m, const void *key);
void     hm_foreach(HashMap *m, void (*cb)(const void *key, void *val, void *ud),
                    void *ud);

#ifdef __cplusplus
}
#endif

#endif // LIB3D_UTIL_H
