#include "lib3d.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==================================================================
 * MESH-BAUSTEINE  (Port von mesh.go)
 *
 * Die erzeugten Arrays werden mit malloc allokiert und mit mesh_free()
 * wieder freigegeben. Zum Aufbau werden in jeder Funktion die exakten
 * Groessen vorab berechnet - es gibt KEINE dynamischen Zwischenspeicher.
 * ================================================================== */

static void tri_push(float *dst, int *i, Vec3 a, Vec3 b, Vec3 c) {
    dst[(*i)++] = (float)a.x; dst[(*i)++] = (float)a.y; dst[(*i)++] = (float)a.z;
    dst[(*i)++] = (float)b.x; dst[(*i)++] = (float)b.y; dst[(*i)++] = (float)b.z;
    dst[(*i)++] = (float)c.x; dst[(*i)++] = (float)c.y; dst[(*i)++] = (float)c.z;
}

static void quad_push(float *dst, int *i, Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
    tri_push(dst, i, a, b, c);
    tri_push(dst, i, a, c, d);
}

static void line_push(float *dst, int *i, Vec3 a, Vec3 b) {
    dst[(*i)++] = (float)a.x; dst[(*i)++] = (float)a.y; dst[(*i)++] = (float)a.z;
    dst[(*i)++] = (float)b.x; dst[(*i)++] = (float)b.y; dst[(*i)++] = (float)b.z;
}

void mesh_free(Mesh *m) {
    if (!m) return;
    free(m->triangles);
    free(m->lines);
    m->triangles = NULL;
    m->lines = NULL;
    m->tri_floats = 0;
    m->line_floats = 0;
}

Mesh make_cube_mesh(double w, double h, double d) {
    double hw = w / 2.0, hh = h / 2.0, hd = d / 2.0;

    Vec3 v[8] = {
        vec3(-hw, -hh, -hd), vec3(hw, -hh, -hd), vec3(hw, hh, -hd), vec3(-hw, hh, -hd),
        vec3(-hw, -hh,  hd), vec3(hw, -hh,  hd), vec3(hw, hh,  hd), vec3(-hw, hh,  hd),
    };
    const int faces[6][4] = {
        {0, 3, 2, 1}, // vorne
        {4, 5, 6, 7}, // hinten
        {0, 4, 7, 3}, // links
        {1, 2, 6, 5}, // rechts
        {0, 1, 5, 4}, // unten
        {3, 7, 6, 2}, // oben
    };
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // vorne
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // hinten
        {0, 4}, {1, 5}, {2, 6}, {3, 7}, // Verbindungen
    };

    Mesh m = {0};
    m.tri_floats = 6 * 2 * 3 * 3;
    m.line_floats = 12 * 2 * 3;
    m.triangles = malloc((size_t) m.tri_floats * sizeof(float));
    m.lines     = malloc((size_t) m.line_floats * sizeof(float));

    int ti = 0, li = 0;
    for (int f = 0; f < 6; f++) {
        quad_push(m.triangles, &ti,
                  v[faces[f][0]], v[faces[f][1]], v[faces[f][2]], v[faces[f][3]]);
    }
    for (int e = 0; e < 12; e++) {
        line_push(m.lines, &li, v[edges[e][0]], v[edges[e][1]]);
    }
    return m;
}

Mesh make_pyramid_mesh(double base, double height) {
    double hb = base / 2.0;

    Vec3 v[5] = {
        vec3(-hb, 0, -hb), vec3(hb, 0, -hb), vec3(hb, 0, hb), vec3(-hb, 0, hb),
        vec3(0, height, 0),
    };
    const int sides[4][3] = {
        {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4},
    };
    const int edges[8][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Basis
        {0, 4}, {1, 4}, {2, 4}, {3, 4}, // Seitenkanten
    };

    Mesh m = {0};
    m.tri_floats = (4 * 3 + 2 * 3) * 3;
    m.line_floats = 8 * 2 * 3;
    m.triangles = malloc((size_t) m.tri_floats * sizeof(float));
    m.lines     = malloc((size_t) m.line_floats * sizeof(float));

    int ti = 0, li = 0;
    for (int t = 0; t < 4; t++) {
        tri_push(m.triangles, &ti, v[sides[t][0]], v[sides[t][1]], v[sides[t][2]]);
    }
    quad_push(m.triangles, &ti, v[3], v[2], v[1], v[0]); // Basis (CCW von unten)
    for (int e = 0; e < 8; e++) {
        line_push(m.lines, &li, v[edges[e][0]], v[edges[e][1]]);
    }
    return m;
}

static Vec3 sphere_point(double radius, int slices, int stacks, int i, int j) {
    double theta = ((double) i / (double) stacks) * M_PI;
    double phi   = ((double) j / (double) slices) * 2.0 * M_PI;
    double r     = radius * sin(theta);
    return vec3(r * cos(phi), radius * cos(theta), r * sin(phi));
}

Mesh make_sphere_mesh(double radius, int slices, int stacks) {
    if (slices < 3) slices = 3;
    if (stacks < 2) stacks = 2;

    // Anzahl Dreiecke: i==0 oder i==stacks-1 -> 1, sonst 2
    int tri_verts = 0;
    for (int i = 0; i < stacks; i++) {
        int per_ring = (i == 0 || i == stacks - 1) ? 1 : 2;
        tri_verts += per_ring * slices;
    }
    // Linien: Meridiane + Breitenringe (ohne Pol-Ringe)
    int line_segs = slices * stacks + (stacks - 1) * slices;

    Mesh m = {0};
    m.tri_floats = tri_verts * 9;
    m.line_floats = line_segs * 6;
    m.triangles = malloc((size_t) m.tri_floats * sizeof(float));
    m.lines     = malloc((size_t) m.line_floats * sizeof(float));

    int ti = 0, li = 0;
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            Vec3 a = sphere_point(radius, slices, stacks, i, j);
            Vec3 b = sphere_point(radius, slices, stacks, i, (j + 1) % slices);
            Vec3 c = sphere_point(radius, slices, stacks, i + 1, (j + 1) % slices);
            Vec3 d = sphere_point(radius, slices, stacks, i + 1, j);
            if (i == 0) {
                tri_push(m.triangles, &ti, a, c, d); // Nordpol
            } else if (i == stacks - 1) {
                tri_push(m.triangles, &ti, a, b, c);  // Suedpol
            } else {
                quad_push(m.triangles, &ti, a, b, c, d);
            }
        }
    }
    // Meridiane (Pol zu Pol)
    for (int j = 0; j < slices; j++) {
        for (int i = 0; i < stacks; i++) {
            line_push(m.lines, &li,
                      sphere_point(radius, slices, stacks, i, j),
                      sphere_point(radius, slices, stacks, i + 1, j));
        }
    }
    // Breitenringe (ohne die zusammenfallenden Pol-Ringe)
    for (int i = 1; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            line_push(m.lines, &li,
                      sphere_point(radius, slices, stacks, i, j),
                      sphere_point(radius, slices, stacks, i, (j + 1) % slices));
        }
    }
    return m;
}

Mesh make_grid_mesh(double size, int cells) {
    if (cells < 1) cells = 1;

    double half = size / 2.0;
    double step = size / (double) cells;

    Mesh m = {0};
    m.tri_floats = 0;
    m.line_floats = 2 * (cells + 1) * 2 * 3;
    m.triangles = NULL;
    m.lines = malloc((size_t) m.line_floats * sizeof(float));

    int li = 0;
    for (int i = 0; i <= cells; i++) {
        double p = -half + (double) i * step;
        // Linie parallel zur Z-Achse und Linie parallel zur X-Achse.
        line_push(m.lines, &li, vec3(p, 0, -half), vec3(p, 0, half));
        line_push(m.lines, &li, vec3(-half, 0, p), vec3(half, 0, p));
    }
    return m;
}

Mesh scale_mesh(Mesh mesh, double s) {
    float f = (float) s;
    Mesh out = {0};
    out.tri_floats = mesh.tri_floats;
    out.line_floats = mesh.line_floats;
    if (mesh.tri_floats > 0) {
        out.triangles = malloc((size_t) mesh.tri_floats * sizeof(float));
        for (int i = 0; i < mesh.tri_floats; i++) {
            out.triangles[i] = mesh.triangles[i] * f;
        }
    }
    if (mesh.line_floats > 0) {
        out.lines = malloc((size_t) mesh.line_floats * sizeof(float));
        for (int i = 0; i < mesh.line_floats; i++) {
            out.lines[i] = mesh.lines[i] * f;
        }
    }
    return out;
}

/* ==================================================================
 * BODY / FACES  (Port von body.go)
 * ================================================================== */

Face *build_faces(const Mesh *mesh, int *out_count) {
    int n = mesh->tri_floats / 9;
    if (n < 0) n = 0;
    Face *faces = malloc((size_t)(n > 0 ? n : 1) * sizeof(Face));
    int cnt = 0;
    for (int i = 0; i + 8 < mesh->tri_floats; i += 9) {
        Face f;
        f.a = vec3(mesh->triangles[i + 0], mesh->triangles[i + 1], mesh->triangles[i + 2]);
        f.b = vec3(mesh->triangles[i + 3], mesh->triangles[i + 4], mesh->triangles[i + 5]);
        f.c = vec3(mesh->triangles[i + 6], mesh->triangles[i + 7], mesh->triangles[i + 8]);
        f.normal = vec3_normalize(vec3_cross(vec3_sub(f.b, f.a), vec3_sub(f.c, f.a)));
        faces[cnt++] = f;
    }
    if (out_count) *out_count = cnt;
    return faces;
}

void faces_free(Face *faces) {
    free(faces);
}

Face transform_face(Face f, const Mat4x4 *model) {
    Face out;
    out.a = vec3_transform(f.a, model);
    out.b = vec3_transform(f.b, model);
    out.c = vec3_transform(f.c, model);
    out.normal = vec3_normalize(vec3_transform_dir(f.normal, model));
    return out;
}

Face *transform_faces(const Face *faces, int count, const Mat4x4 *model,
                      int *out_count) {
    if (count < 0) count = 0;
    Face *out = malloc((size_t)(count > 0 ? count : 1) * sizeof(Face));
    for (int i = 0; i < count; i++) {
        out[i] = transform_face(faces[i], model);
    }
    if (out_count) *out_count = count;
    return out;
}

// Moeller-Trumbore: Strecke [p1, p1+dir] gegen Dreieck (a,b,c).
// Liefert 1 und t (Parameter 0..1) bei Treffer.
static int ray_triangle(Vec3 p1, Vec3 dir, Vec3 a, Vec3 b, Vec3 c,
                        double *out_t) {
    Vec3 e1 = vec3_sub(b, a);
    Vec3 e2 = vec3_sub(c, a);
    Vec3 p  = vec3_cross(dir, e2);
    double det = vec3_dot(e1, p);
    if (fabs(det) < 1e-12) return 0; // Strecke parallel zur Face
    double inv = 1.0 / det;
    Vec3 tvec = vec3_sub(p1, a);
    double u = vec3_dot(tvec, p) * inv;
    if (u < 0.0 || u > 1.0) return 0;
    Vec3 q = vec3_cross(tvec, e1);
    double v = vec3_dot(dir, q) * inv;
    if (v < 0.0 || u + v > 1.0) return 0;
    double t = vec3_dot(e2, q) * inv;
    if (t < 0.0 || t > 1.0) return 0;
    *out_t = t;
    return 1;
}

RayHit raycast_segment(Vec3 p1, Vec3 p2, const Face *faces, int count) {
    RayHit best = {0};
    Vec3 dir = vec3_sub(p2, p1);
    if (vec3_squared_length(dir) <= 0.0) return best;

    double bestT = 1.0;
    for (int i = 0; i < count; i++) {
        double t = 0.0;
        if (!ray_triangle(p1, dir, faces[i].a, faces[i].b, faces[i].c, &t)) continue;
        if (t > bestT) continue;
        bestT = t;
        best.isHit = 1;
        best.t = t;
        best.point = vec3_add(p1, vec3_scale(dir, t));
        best.normal = faces[i].normal;
    }
    return best;
}

// Naechster Punkt auf dem Dreieck (a,b,c) zu p (Ericson, RTCD 5.1.5).
static Vec3 closest_point_on_triangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c) {
    Vec3 ab = vec3_sub(b, a);
    Vec3 ac = vec3_sub(c, a);
    Vec3 ap = vec3_sub(p, a);
    double d1 = vec3_dot(ab, ap);
    double d2 = vec3_dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a; // Ecke A

    Vec3 bp = vec3_sub(p, b);
    double d3 = vec3_dot(ab, bp);
    double d4 = vec3_dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return b; // Ecke B

    double vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        return vec3_add(a, vec3_scale(ab, d1 / (d1 - d3))); // Kante AB
    }

    Vec3 cp = vec3_sub(p, c);
    double d5 = vec3_dot(ab, cp);
    double d6 = vec3_dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return c; // Ecke C

    double vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        return vec3_add(a, vec3_scale(ac, d2 / (d2 - d6))); // Kante AC
    }

    double va = d3 * d6 - d5 * d4;
    if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
        double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return vec3_add(b, vec3_scale(vec3_sub(c, b), w)); // Kante BC
    }

    // Inneres: baryzentrische Koordinaten (u, v, w), u = 1 - v - w.
    double denom = 1.0 / (va + vb + vc);
    double v = vb * denom;
    double w = vc * denom;
    return vec3_add(a, vec3_add(vec3_scale(ab, v), vec3_scale(ac, w)));
}

ClosestFaceHit closest_point_to_face(Vec3 p, Face face) {
    ClosestFaceHit hit;
    hit.point = closest_point_on_triangle(p, face.a, face.b, face.c);
    hit.normal = face.normal;
    hit.distance = vec3_distance(p, hit.point);
    hit.signedDistance = vec3_dot(vec3_sub(p, hit.point), face.normal);
    return hit;
}

Mat4x4 body_model_matrix(const Body *b) {
    Mat4x4 t = mat4_translate(b->pos.x, b->pos.y, b->pos.z);
    if (b->rot.x == 0.0 && b->rot.y == 0.0 && b->rot.z == 0.0) {
        return t;
    }
    Mat4x4 r = mat4_rotate(b->rot.x, b->rot.y, b->rot.z);
    return mat4_mult(&t, &r);
}

Face *body_world_faces(const Body *b, int *out_count) {
    Mat4x4 model = body_model_matrix(b);
    return transform_faces(b->faces, b->face_count, &model, out_count);
}

Body make_body(Mesh geometry, Vec3 pos, Vec3 rot) {
    Body b;
    b.geometry = geometry;
    b.pos = pos;
    b.rot = rot;
    b.faces = build_faces(&geometry, &b.face_count);
    return b;
}

void body_free(Body *b) {
    if (!b) return;
    faces_free(b->faces);
    b->faces = NULL;
    b->face_count = 0;
}

/* ==================================================================
 * MESH-LADEN  (Port von mesh_io.go, Wavefront OBJ)
 * ================================================================== */

typedef struct {
    // Anzahl-Phase
    int vertex_count;
    int tri_floats;
    int line_floats;
    // Fuell-Phase
    Vec3  *verts;
    int    vert_cap;
    int    vert_next;
    float *out_tri;
    int    tri_cap;
    int    tri_next;
    float *out_lines;
    int    line_cap;
    int    line_next;
} ObjState;

// Parst ein Face-/Linien-Token ("i", "i/j", "i/j/k", "i//k") in einen
// 0-basierten Index. Liefert L3D_OBJ_OK oder L3D_OBJ_ERR_INDEX.
static int obj_index(const char *tok, int vertexCount, int *out) {
    const char *slash = strchr(tok, '/');
    size_t len = slash ? (size_t)(slash - tok) : strlen(tok);
    if (len == 0) return L3D_OBJ_ERR_INDEX;

    char buf[64];
    if (len >= sizeof(buf)) return L3D_OBJ_ERR_INDEX;
    memcpy(buf, tok, len);
    buf[len] = '\0';

    errno = 0;
    char *end = NULL;
    long n = strtol(buf, &end, 10);
    if (errno != 0 || end == buf || *end != '\0') return L3D_OBJ_ERR_INDEX;

    if (n < 0) {
        n = vertexCount + n; // negativ: relativ zum Ende
    } else {
        n = n - 1;                      // OBJ ist 1-basiert
    }
    if (n < 0 || n >= vertexCount) return L3D_OBJ_ERR_INDEX;
    *out = (int) n;
    return L3D_OBJ_OK;
}

static void obj_emit_tri(ObjState *st, Vec3 a, Vec3 b, Vec3 c) {
    tri_push(st->out_tri, &st->tri_next, a, b, c);
}

static void obj_emit_line(ObjState *st, Vec3 a, Vec3 b) {
    line_push(st->out_lines, &st->line_next, a, b);
}

// Verarbeitet eine OBJ-Zeile. liefert L3D_OBJ_OK oder einen Fehlercode.
static int obj_handle_line(char *line, ObjState *st, int line_no,
                           int fill, int *err_line) {
    (void) line_no;
    char *toks[256];
    int nt = 0;
    char *p = line;
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;
        if (!*p) break;
        if (nt >= 256) return L3D_OBJ_ERR_INDEX;
        toks[nt++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r') p++;
        if (*p) *p++ = '\0';
    }
    if (nt == 0) return L3D_OBJ_OK;
    if (err_line) *err_line = line_no;

    if (strcmp(toks[0], "v") == 0) {
        if (nt < 4) return L3D_OBJ_ERR_VERTEX;
        errno = 0;
        char *e1 = NULL, *e2 = NULL, *e3 = NULL;
        double x = strtod(toks[1], &e1);
        double y = strtod(toks[2], &e2);
        double z = strtod(toks[3], &e3);
        if (errno != 0 || e1 == toks[1] || e2 == toks[2] || e3 == toks[3]) {
            return L3D_OBJ_ERR_VERTEX;
        }
        if (fill) {
            if (st->vert_next >= st->vert_cap) return L3D_OBJ_ERR_VERTEX;
            st->verts[st->vert_next++] = vec3(x, y, z);
        } else {
            st->vertex_count++;
        }
        return L3D_OBJ_OK;
    }

    if (strcmp(toks[0], "f") == 0 || strcmp(toks[0], "l") == 0) {
        int idx[256];
        int ni = 0;
        // Negative Indizes sind relativ zur Anzahl der BIS JETZT gelesenen
        // Vertices (wie in Go: len(verts) zur Parse-Zeit). Daher im
        // Fill-Modus vert_next, im Zaehl-Modus vertex_count als Basis.
        int base = fill ? st->vert_next : st->vertex_count;
        for (int i = 1; i < nt; i++) {
            int out = 0;
            int r = obj_index(toks[i], base, &out);
            if (r != L3D_OBJ_OK) return r;
            idx[ni++] = out;
        }
        if (toks[0][0] == 'f') {
            // Fan-Triangulierung: (idx0, idx[j], idx[j+1])
            if (fill) {
                if (st->tri_next + (ni - 2) * 9 > st->tri_cap) return L3D_OBJ_ERR_INDEX;
                for (int j = 1; j + 1 < ni; j++) {
                    obj_emit_tri(st, st->verts[idx[0]], st->verts[idx[j]],
                                 st->verts[idx[j + 1]]);
                }
            } else {
                if (ni >= 3) st->tri_floats += (ni - 2) * 9;
            }
        } else {
            // Aufeinanderfolgende Paare
            if (fill) {
                if (st->line_next + (ni - 1) * 6 > st->line_cap) return L3D_OBJ_ERR_INDEX;
                for (int j = 0; j + 1 < ni; j++) {
                    obj_emit_line(st, st->verts[idx[j]], st->verts[idx[j + 1]]);
                }
            } else {
                if (ni >= 2) st->line_floats += (ni - 1) * 6;
            }
        }
        return L3D_OBJ_OK;
    }

    // vn, vt, g, o, s, mtllib, usemtl, ... ignoriert.
    return L3D_OBJ_OK;
}

// Verarbeitet alle Zeilen des Puffers NACH ZWEIMAL in fill/count-Modus.
// Puffer wird NICHT zerstoert (sauberer als strtok_r).
#define OBJ_MAX_LINE 8192

static int obj_process_lines(const char *buf, ObjState *st, int fill,
                             int *err_line) {
    int line_no = 0;
    const char *cursor = buf;
    while (*cursor) {
        const char *nl = strchr(cursor, '\n');
        size_t len = nl ? (size_t)(nl - cursor) : strlen(cursor);
        if (len >= OBJ_MAX_LINE) return L3D_OBJ_ERR_INDEX;

        char line[OBJ_MAX_LINE];
        memcpy(line, cursor, len);
        line[len] = '\0';

        line_no++;
        int r = obj_handle_line(line, st, line_no, fill, err_line);
        if (r != L3D_OBJ_OK) return r;

        if (!nl) break;
        cursor = nl + 1;
    }
    return L3D_OBJ_OK;
}

int load_obj(const char *path, Mesh *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "OBJ: Datei '%s' nicht gefunden\n", path);
        return L3D_OBJ_ERR_FILE;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t) sz + 2);
    size_t got = fread(buf, 1, (size_t) sz, f);
    fclose(f);
    if (got != (size_t) sz) {
        free(buf);
        return L3D_OBJ_ERR_FILE;
    }
    buf[sz] = '\n';
    buf[sz + 1] = '\0';

    // Pass 1: zaehlen + validieren
    ObjState st = {0};
    int err = L3D_OBJ_OK;
    int err_line = 0;
    err = obj_process_lines(buf, &st, 0, &err_line);
    if (err != L3D_OBJ_OK) {
        fprintf(stderr, "OBJ Zeile %d: Fehler %d (%s)\n", err_line, err, path);
        free(buf);
        return err;
    }

    // Pass 2: Arrays exakt allozieren und fuellen.
    Mesh m = {0};
    m.tri_floats = st.tri_floats;
    m.line_floats = st.line_floats;
    if (m.tri_floats > 0) m.triangles = malloc((size_t) m.tri_floats * sizeof(float));
    if (m.line_floats > 0) m.lines     = malloc((size_t) m.line_floats * sizeof(float));

    st.vert_cap = st.vertex_count;
    st.verts    = st.vertex_count > 0 ? malloc((size_t) st.vertex_count * sizeof(Vec3)) : NULL;
    st.tri_cap  = m.tri_floats;
    st.tri_next = 0;
    st.line_cap = m.line_floats;
    st.line_next = 0;
    st.out_tri    = m.triangles;
    st.out_lines  = m.lines;

    err = L3D_OBJ_OK;
    err_line = 0;
    err = obj_process_lines(buf, &st, 1, &err_line);
    free(buf);
    free(st.verts);

    if (err != L3D_OBJ_OK ||
        st.tri_next != m.tri_floats || st.line_next != m.line_floats) {
        fprintf(stderr, "OBJ Zeile %d: Fehler beim Fuellen (%s)\n", err_line, path);
        mesh_free(&m);
        return err != L3D_OBJ_OK ? err : L3D_OBJ_ERR_FILE;
    }

    *out = m;
    return L3D_OBJ_OK;
}
