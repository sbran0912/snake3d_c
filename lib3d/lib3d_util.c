#include "lib3d_util.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ==================================================================
 * VEC3  (Port von vec.go)
 * ================================================================== */

Vec3 vec3(double x, double y, double z) {
    Vec3 v = {x, y, z};
    return v;
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 vec3_scale(Vec3 a, double s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

Vec3 vec3_mag(Vec3 a, double m) {
    double length = vec3_length(a);
    if (length == 0.0) return vec3(0, 0, 0);
    return vec3_scale(a, m / length);
}

Vec3 vec3_limit(Vec3 a, double maxLen) {
    double mSq = vec3_squared_length(a);
    if (mSq > maxLen * maxLen) {
        return vec3_scale(a, maxLen / sqrt(mSq));
    }
    return a;
}

Vec3 vec3_negate(Vec3 a) {
    return vec3(-a.x, -a.y, -a.z);
}

double vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

double vec3_squared_length(Vec3 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

double vec3_length(Vec3 a) {
    return sqrt(vec3_squared_length(a));
}

double vec3_distance(Vec3 a, Vec3 b) {
    return vec3_length(vec3_sub(a, b));
}

Vec3 vec3_normalize(Vec3 a) {
    double length = vec3_length(a);
    if (length == 0.0) return vec3(0, 0, 0);
    return vec3_scale(a, 1.0 / length);
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, double t) {
    return vec3_add(a, vec3_scale(vec3_sub(b, a), t));
}

Vec3 vec3_clone(Vec3 a) {
    return a;
}

int vec3_equals(Vec3 a, Vec3 b, double epsilon) {
    return fabs(a.x - b.x) < epsilon &&
           fabs(a.y - b.y) < epsilon &&
           fabs(a.z - b.z) < epsilon;
}

Vec3 vec3_transform(Vec3 v, const Mat4x4 *m) {
    return vec3(
        m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z + m->m[0][3],
        m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z + m->m[1][3],
        m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z + m->m[2][3]);
}

Vec3 vec3_transform_dir(Vec3 v, const Mat4x4 *m) {
    return vec3(
        v.x * m->m[0][0] + v.y * m->m[0][1] + v.z * m->m[0][2],
        v.x * m->m[1][0] + v.y * m->m[1][1] + v.z * m->m[1][2],
        v.x * m->m[2][0] + v.y * m->m[2][1] + v.z * m->m[2][2]);
}

/* ==================================================================
 * MATRIX  (Port von mat4.go, zeilen-major double[4][4])
 * ================================================================== */

Mat4x4 mat4_identity(void) {
    Mat4x4 r = {0};
    r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0;
    return r;
}

Mat4x4 mat4_translate(double dx, double dy, double dz) {
    Mat4x4 r = mat4_identity();
    r.m[0][3] = dx;
    r.m[1][3] = dy;
    r.m[2][3] = dz;
    return r;
}

Mat4x4 mat4_mult(const Mat4x4 *a, const Mat4x4 *b) {
    Mat4x4 r = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum += a->m[i][k] * b->m[k][j];
            }
            r.m[i][j] = sum;
        }
    }
    return r;
}

Mat4x4 mat4_rotate(double ax, double ay, double az) {
    double cax = cos(ax), sax = sin(ax);
    double cay = cos(ay), say = sin(ay);
    double caz = cos(az), saz = sin(az);

    Mat4x4 rx = {0};
    rx.m[0][0] = 1; rx.m[1][1] = cax; rx.m[1][2] = -sax;
    rx.m[2][1] = sax; rx.m[2][2] = cax; rx.m[3][3] = 1;

    Mat4x4 ry = {0};
    ry.m[0][0] = cay; ry.m[0][2] = say;
    ry.m[1][1] = 1; ry.m[2][0] = -say; ry.m[2][2] = cay; ry.m[3][3] = 1;

    Mat4x4 rz = {0};
    rz.m[0][0] = caz; rz.m[0][1] = -saz;
    rz.m[1][0] = saz; rz.m[1][1] = caz; rz.m[2][2] = 1; rz.m[3][3] = 1;

    Mat4x4 ry_rx = mat4_mult(&ry, &rx);
    return mat4_mult(&rz, &ry_rx);
}

Mat4x4 mat4_view_world(const Mat4x4 *view, const Mat4x4 *world) {
    return mat4_mult(view, world);
}

Mat4x4 mat4_lookat(Vec3 cameraPos, Vec3 target, Vec3 up) {
    Vec3 forward = vec3_normalize(vec3_sub(target, cameraPos));
    Vec3 right   = vec3_normalize(vec3_cross(forward, up));
    Vec3 realUp  = vec3_cross(right, forward);

    Mat4x4 r = {0};
    r.m[0][0] = right.x;  r.m[0][1] = right.y;  r.m[0][2] = right.z;
    r.m[0][3] = -vec3_dot(right, cameraPos);
    r.m[1][0] = realUp.x; r.m[1][1] = realUp.y; r.m[1][2] = realUp.z;
    r.m[1][3] = -vec3_dot(realUp, cameraPos);
    r.m[2][0] = forward.x; r.m[2][1] = forward.y; r.m[2][2] = forward.z;
    r.m[2][3] = -vec3_dot(forward, cameraPos);
    r.m[3][3] = 1.0;
    return r;
}

Vec3 world_to_camera(Vec3 point, const Mat4x4 *view, const Mat4x4 *world) {
    Mat4x4 vw = mat4_mult(view, world);
    return vec3_transform(point, &vw);
}

Vec3 rotate_around(Vec3 point, Vec3 pivot, const Mat4x4 *rotation) {
    Vec3 rel     = vec3_sub(point, pivot);
    Vec3 rotated = vec3_transform(rel, rotation);
    return vec3_add(rotated, pivot);
}

Mat4x4 mat4_perspective(double fovY, double aspect, double near, double far) {
    double f = 1.0 / tan(fovY / 2.0);
    Mat4x4 r = {0};
    r.m[0][0] = f / aspect;
    r.m[1][1] = f;
    r.m[2][2] = (far + near) / (far - near);
    r.m[2][3] = -2.0 * near * far / (far - near);
    r.m[3][2] = 1.0;
    return r;
}

/* ==================================================================
 * FARBE  (Port von color.go)
 * ================================================================== */

ColorState parse_hex(const char *hex) {
    const char *h = hex;
    if (*h == '#') h++;
    size_t len = strlen(h);

    char buf[7] = {0, 0, 0, 0, 0, 0, 0};
    if (len == 3) {
        buf[0] = h[0]; buf[1] = h[0];
        buf[2] = h[1]; buf[3] = h[1];
        buf[4] = h[2]; buf[5] = h[2];
    } else if (len == 6) {
        memcpy(buf, h, 6);
    } else {
        return (ColorState){1, 1, 1, 1};
    }

    errno = 0;
    char *end = NULL;
    unsigned long n = strtoul(buf, &end, 16);
    if (errno != 0 || end == buf || *end != '\0') {
        return (ColorState){1, 1, 1, 1};
    }
    return (ColorState){
        ((n >> 16) & 0xff) / 255.0,
        ((n >> 8) & 0xff) / 255.0,
        (n & 0xff) / 255.0,
        1
    };
}

ColorState parse_gray(double gray) {
    double g = gray / 255.0;
    return (ColorState){g, g, g, 1};
}

ColorState parse_rgb(double r, double g, double b) {
    return (ColorState){r / 255.0, g / 255.0, b / 255.0, 1};
}

ColorState parse_rgba(double r, double g, double b, double a) {
    return (ColorState){r / 255.0, g / 255.0, b / 255.0, a / 255.0};
}

int col_eq(ColorState a, ColorState b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

ColorInput color_hex(const char *hex)          { return (ColorInput){1, parse_hex(hex)}; }
ColorInput color_gray(double v)                { return (ColorInput){1, parse_gray(v)}; }
ColorInput color_rgb(double r, double g, double b) {
    return (ColorInput){1, parse_rgb(r, g, b)};
}
ColorInput color_rgba(double r, double g, double b, double a) {
    return (ColorInput){1, parse_rgba(r, g, b, a)};
}
ColorInput color_of(ColorState c)              { return (ColorInput){1, c}; }

/* ==================================================================
 * MATH-HILFEN  (Port von math.go)
 * ================================================================== */

Vec2 project(double fov, Vec3 v) {
    double s = fov / (fov + v.z);
    return (Vec2){v.x * s, v.y * s, s};
}

double l3d_constrain(double value, double lo, double hi) {
    return fmax(lo, fmin(hi, value));
}

double l3d_random_float(double lo, double hi) {
    return (rand() / ((double)RAND_MAX + 1.0)) * (hi - lo) + lo;
}

int l3d_random(int n1, int n2) {
    return (int) floor(l3d_random_float(0.0, 1.0) * (double)(n2 - n1) + (double)n1);
}

float limit_num(float number, float limit) {
    int vorzeichen = (number < 0) ? -1 : 1;
    float numberMag = number * vorzeichen;
    if (numberMag > limit) numberMag = limit;

    return numberMag * vorzeichen;
}

//dynamisches Array
void* arr_create(size_t capacity, size_t elemsize) {
    size_t *raw = malloc(3 * sizeof(size_t) + (capacity * elemsize));
    raw[0] = 0;                 // Number of elements currently in the array
    raw[1] = capacity;         // capacity - how many elements can the array currently store
    raw[2] = elemsize;           // Size of elements
    return (void*) &raw[3];     // Pointer to the beginning of the array
}

size_t arr_len(void* arr) {
    return *((size_t*)arr - 3);
}

size_t arr_cap(void* arr) {
    return *((size_t*)arr - 2);
}

void* arr_push(void* arr, void* val) {
    size_t* raw = (size_t*)(void*)arr - 3;
    size_t len = raw[0];
    size_t cap = raw[1];
    size_t elemsize = raw[2];

    len++;                  //increase counter
    raw[0] = len;           //save counter in array

    if (len > cap) {
        size_t new_cap = cap + 10;
        raw[1] = new_cap;      //calculate new capacity
        raw = realloc(raw, 3 * sizeof(size_t) + (new_cap * elemsize));
        arr = (void*)&raw[3]; //Array-Pointer wird wieder auf richtigen Startpunkt gestellt
    }

    char* new_item = (char*)&raw[3] + ((len-1) * elemsize);   //count 1 = index 0, count 2 = index 1 etc.
    memcpy(new_item, val, elemsize);
    return arr;
}

void arr_pop(void* arr, size_t idx) {
    size_t* raw = (size_t*)(void*)arr - 3;
    size_t len = raw[0];
    size_t elemsize = raw[2];
    while (idx < len -1) {
        char* val = (char*)&raw[3] + ((idx + 1) * elemsize); //Pointer auf nächstes Element
        char* upd_item = (char*)&raw[3] + (idx * elemsize); //Pointer aktuelles Element
        memcpy(upd_item, val, elemsize); //Kopiere Wert vom nächsten Element auf aktuelles Element
        idx++;
    }
    len--;
    raw[0] = len;
}

void* arr_reduce(void* arr) {
    size_t* raw = (size_t*)(void*)arr - 3;
    size_t len = raw[0];
    size_t cap = raw[1];
    size_t elemsize = raw[2];

    if (len < cap) {
        cap = len;
        raw[1] = cap; //Kapazität auf Länge setzen
        raw = realloc(raw, 3 * sizeof(size_t) + (len * elemsize)); //Speicherbereich kürzen
        arr = (void*)&raw[3]; //Array-Pointer wird wieder auf richtigen Startpunkt gestellt
    }
    return arr;
}

void arr_delete(void* arr) {
    size_t* raw = (size_t*)(void*)arr - 3;
    raw[0] = 0;
}

void arr_free(void* arr) {
    size_t* raw = (size_t*)(void*)arr - 3;
    free(raw);
}

/* ==================================================================
 * HASH-MAP  (generisch, Ersatz fuer Go's map[K]V; frueher in lib_core.c)
 *
 * Verkettet in Buckets; waechst automatisch, sobald der Fuellgrad
 * ~0.75 ueberschreitet. Schluessel und Werte werden als rohe Bytes
 * kopiert (memcpy). Optional koennen eigene Hash-/Vergleichsfunktionen
 * uebergeben werden (NULL = FNV-1a ueber die Bytes bzw. memcmp).
 * ================================================================== */

struct HmBucket {
    void *key;
    void *val;
    struct HmBucket *next;
};

static uint64_t hm_fnv(const void *key, size_t len) {
    const unsigned char *p = (const unsigned char*) key;
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t hm_hash(const HashMap *m, const void *key) {
    if (m->hash) return m->hash(key);
    return hm_fnv(key, m->keysize);
}

static int hm_eq(const HashMap *m, const void *ka, const void *kb) {
    if (m->eq) return m->eq(ka, kb);
    return memcmp(ka, kb, m->keysize) == 0;
}

HashMap* hm_create(int capacity, size_t keysize, size_t valsize,
                   HashMap_HashFn hash, HashMap_EqFn eq) {
    if (capacity < 8) capacity = 8;
    HashMap *m = malloc(sizeof(HashMap));
    m->nbuckets = (size_t) capacity;
    m->buckets = calloc(m->nbuckets, sizeof(HmBucket*));
    m->count = 0;
    m->keysize = keysize;
    m->valsize = valsize;
    m->hash = hash;
    m->eq = eq;
    return m;
}

static void hm_rehash(HashMap *m, size_t new_cap) {
    HmBucket **nb = calloc(new_cap, sizeof(HmBucket*));
    for (size_t i = 0; i < m->nbuckets; i++) {
        HmBucket *b = m->buckets[i];
        while (b) {
            HmBucket *next = b->next;
            size_t idx = (size_t)(hm_hash(m, b->key) % new_cap);
            b->next = nb[idx];
            nb[idx] = b;
            b = next;
        }
    }
    free(m->buckets);
    m->buckets = nb;
    m->nbuckets = new_cap;
}

static void hm_grow(HashMap *m) {
    if (m->count * 4 >= m->nbuckets * 3) {
        hm_rehash(m, m->nbuckets * 2 + 1);
    }
}

void hm_free(HashMap *m) {
    if (!m) return;
    hm_clear(m);
    free(m->buckets);
    free(m);
}

void hm_clear(HashMap *m) {
    for (size_t i = 0; i < m->nbuckets; i++) {
        HmBucket *b = m->buckets[i];
        while (b) {
            HmBucket *next = b->next;
            free(b->key);
            free(b->val);
            free(b);
            b = next;
        }
        m->buckets[i] = NULL;
    }
    m->count = 0;
}

size_t hm_size(HashMap *m) {
    return m ? m->count : 0;
}

int hm_contains(HashMap *m, const void *key) {
    size_t idx = (size_t)(hm_hash(m, key) % m->nbuckets);
    for (HmBucket *b = m->buckets[idx]; b; b = b->next) {
        if (hm_eq(m, key, b->key)) return 1;
    }
    return 0;
}

void *hm_get(HashMap *m, const void *key) {
    size_t idx = (size_t)(hm_hash(m, key) % m->nbuckets);
    for (HmBucket *b = m->buckets[idx]; b; b = b->next) {
        if (hm_eq(m, key, b->key)) return b->val;
    }
    return NULL;
}

void hm_put(HashMap *m, const void *key, const void *val) {
    size_t idx = (size_t)(hm_hash(m, key) % m->nbuckets);
    for (HmBucket *b = m->buckets[idx]; b; b = b->next) {
        if (hm_eq(m, key, b->key)) {
            memcpy(b->val, val, m->valsize);
            return;
        }
    }
    hm_grow(m);
    idx = (size_t)(hm_hash(m, key) % m->nbuckets);
    HmBucket *b = malloc(sizeof(HmBucket));
    b->key = malloc(m->keysize);
    b->val = malloc(m->valsize);
    memcpy(b->key, key, m->keysize);
    memcpy(b->val, val, m->valsize);
    b->next = m->buckets[idx];
    m->buckets[idx] = b;
    m->count++;
}

void hm_remove(HashMap *m, const void *key) {
    size_t idx = (size_t)(hm_hash(m, key) % m->nbuckets);
    HmBucket **link = &m->buckets[idx];
    while (*link) {
        HmBucket *b = *link;
        if (hm_eq(m, key, b->key)) {
            *link = b->next;
            free(b->key);
            free(b->val);
            free(b);
            m->count--;
            return;
        }
        link = &b->next;
    }
}

void hm_foreach(HashMap *m, void (*cb)(const void *key, void *val, void *ud),
                void *ud) {
    for (size_t i = 0; i < m->nbuckets; i++) {
        for (HmBucket *b = m->buckets[i]; b; b = b->next) {
            cb(b->key, b->val, ud);
        }
    }
}
