#include "lib3d.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "glcore.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==================================================================
 * GLSL-SHADER  (Port von shader.go)
 * ================================================================== */

static const char *VERT_SRC =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in mat4 aInstModel;\n"
    "layout(location = 6) in vec4 aInstColor;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "uniform float uPointSize;\n"
    "uniform int uInstanced;\n"
    "out vec3 vCamPos;\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "    vec4 world;\n"
    "    if (uInstanced == 1) {\n"
    "        world = aInstModel * vec4(aPos, 1.0);\n"
    "        vColor = aInstColor;\n"
    "    } else {\n"
    "        world = vec4(aPos, 1.0);\n"
    "        vColor = aColor;\n"
    "    }\n"
    "    vec4 camPos = uView * world;\n"
    "    vCamPos = camPos.xyz;\n"
    "    gl_Position = uProjection * camPos;\n"
    "    gl_PointSize = uPointSize;\n"
    "}\n";

static const char *FRAG_SRC =
    "#version 330 core\n"
    "in vec3 vCamPos;\n"
    "in vec4 vColor;\n"
    "out vec4 fragColor;\n"
    "uniform int   uMode;\n"
    "uniform vec4  uColor;\n"
    "uniform vec4  uColor2;\n"
    "uniform float uTime;\n"
    "uniform vec3  uShapeCenter;\n"
    "uniform float uShapeRadius;\n"
    "uniform float uFogNear;\n"
    "uniform float uFogFar;\n"
    "uniform vec4  uFogColor;\n"
    "uniform int   uLighted;\n"
    "uniform vec3  uLightDir;\n"
    "void main() {\n"
    "  vec3 base = uColor.rgb;\n"
    "  if (uMode == 0) {\n"
    "  } else if (uMode == 1) {\n"
    "    float d = distance(vCamPos, uShapeCenter);\n"
    "    float t = clamp(d / max(uShapeRadius, 1.0), 0.0, 1.0);\n"
    "    base = mix(uColor, uColor2, t).rgb;\n"
    "  } else if (uMode == 2) {\n"
    "    float brightness = 0.6 + 0.4 * sin(uTime * 3.0);\n"
    "    base = uColor.rgb * brightness;\n"
    "  }\n"
    "  base *= vColor.rgb;\n"
    "  if (uLighted == 1) {\n"
    "    vec3 n = normalize(cross(dFdy(vCamPos), dFdx(vCamPos)));\n"
    "    float diff = max(dot(n, normalize(uLightDir)), 0.0);\n"
    "    base *= 0.25 + 0.75 * diff;\n"
    "  }\n"
    "  float depth = vCamPos.z;\n"
    "  float fog_t = clamp((depth - uFogNear) / (uFogFar - uFogNear), 0.0, 1.0);\n"
    "  fragColor = mix(vec4(base, uColor.a * vColor.a), uFogColor, fog_t);\n"
    "}\n";

/* ==================================================================
 * FIXE PUFFER  (keine dynamischen Datenstrukturen im Renderer)
 * ================================================================== */
#define MAX_BATCH_VERTS (1u << 19)   // ~74k Vertices (2 MiB)
#define MAX_BATCH_CMDS  8192
#define MAX_INST_GEOM   (1u << 15)   // Osteo-Geometrie der Instanzen
#define MAX_INST_DATA   (1u << 19)   // ~26k Instanzen (2 MiB)
#define MAX_INST_CMDS   1024
#define K_MAX_KEYS      512

typedef struct {
    uint32_t mode;
    int      first;   // Start in batchVerts (Vertex-Nummer)
    int      count;   // Anzahl Vertices
    ColorState  col;
    EffectMode  effect;
    ColorState  grad2;
    double pointSize;
    double lineW;
    double center[3];
    double radius;
    float  view[16];
    float  proj[16];
    int    lit;
} DrawCmd;

typedef struct {
    uint32_t mode;
    int      first, count;
    int      instFirst;
    int      instCount;
    EffectMode effect;
    ColorState grad2;
    double pointSize;
    double lineW;
    double center[3];
    double radius;
    float  view[16];
    float  proj[16];
    int    lit;
} InstCmd;

typedef struct {
    ColorState fill;
    ColorState stroke;
    double     lineW;
    EffectMode effect;
    ColorState grad2;
} DrawState;

/* --- Fenster / GL --- */
static GLFWwindow *window    = NULL;
static GLuint      program   = 0;
static GLuint      vao       = 0;
static GLuint      vbo       = 0;
static GLuint      vboGeom   = 0;
static GLuint      vboInst   = 0;
static int         frameW    = 0;
static int         frameH    = 0;

/* --- Shader-Locations --- */
static GLint locPos, locColorAttr, locInstModel, locInstColor;
static GLint locInstanced, locView, locProjection, locPointSize;
static GLint locMode, locColor, locColor2, locTime;
static GLint locCenter, locRadius, locFogNear, locFogFar, locFogColor;
static GLint locLighted, locLightDir;

/* --- Animation --- */
static int    looping   = 1;
static double startTime = 0.0;

/* --- Vollbild --- */
static int fullscreen             = 0;
static int toggleFullscreenWanted = 0;
static int windowedPos[2];
static int windowedSize[2];

/* --- Maus --- */
static double mouseX      = 0.0;
static double mouseY      = 0.0;
static int    mouseStatus = 0;

/* --- Tastatur --- */
static int keyDown[K_MAX_KEYS];
static int keyPressed[K_MAX_KEYS];

/* --- Zeichenzustand --- */
static DrawState state = {
    {1, 1, 1, 1},  // fill
    {0, 0, 0, 1},  // stroke
    1.0,           // lineW
    EFFECT_FLAT,   // effect
    {0, 0, 0, 1},  // grad2
};

/* --- Batching --- */
static float   batchVerts[MAX_BATCH_VERTS];
static int     batchVertCount = 0;
static DrawCmd batchCmds[MAX_BATCH_CMDS];
static int     batchCmdCount  = 0;
static int     batchCapBytes  = 0;   // aktuell allozierte Byte-Groesse des VBO
static int     batchSort      = 0;

/* --- Instancing --- */
static float   instGeom[MAX_INST_GEOM];
static int     instGeomCount = 0;
static float   instData[MAX_INST_DATA];
static int     instDataCount = 0;
static InstCmd instCmds[MAX_INST_CMDS];
static int     instCmdCount  = 0;
static int     geomCapBytes  = 0;
static int     instCapBytes  = 0;

/* --- Zuletzt gesetzte Uniform-Werte (Batch-Snapshot) --- */
static const float identity16[16] = {
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1
};
static float viewUniform[16];
static float projUniform[16];
static double pointSizeVal = 4.0;
static double gradCenter[3] = {0, 0, 0};
static double gradRadius    = 1.0;

/* ==================================================================
 * HILFSFUNKTIONEN (intern)
 * ================================================================== */

// Zeilen-major double[4][4] -> Spalten-major float[16] fuer OpenGL.
static void flatten_matrix(const Mat4x4 *m, float *out) {
    out[0]  = (float)m->m[0][0]; out[1]  = (float)m->m[1][0];
    out[2]  = (float)m->m[2][0]; out[3]  = (float)m->m[3][0];
    out[4]  = (float)m->m[0][1]; out[5]  = (float)m->m[1][1];
    out[6]  = (float)m->m[2][1]; out[7]  = (float)m->m[3][1];
    out[8]  = (float)m->m[0][2]; out[9]  = (float)m->m[1][2];
    out[10] = (float)m->m[2][2]; out[11] = (float)m->m[3][2];
    out[12] = (float)m->m[0][3]; out[13] = (float)m->m[1][3];
    out[14] = (float)m->m[2][3]; out[15] = (float)m->m[3][3];
}

static int effect_to_mode(EffectMode effect) {
    // enum-Werte sind bereits 0/1/2 wie in Go; trotzdem explizit.
    switch (effect) {
    case EFFECT_GRADIENT: return 1;
    case EFFECT_PULSE:    return 2;
    default:              return 0;
    }
}

static int arr16_eq(const float *a, const float *b) {
    for (int i = 0; i < 16; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

// Zwei aufeinanderfolgende Befehle mit identischem Zustand zusammenfassen.
// LINE_LOOP wird nie zusammengefasst (schliesst sich selbst).
static int can_merge(const DrawCmd *a, const DrawCmd *b) {
    return a->mode == b->mode &&
           a->mode != GL_LINE_LOOP &&
           col_eq(a->col, b->col) &&
           a->effect == b->effect &&
           col_eq(a->grad2, b->grad2) &&
           a->pointSize == b->pointSize &&
           a->lineW == b->lineW &&
           a->center[0] == b->center[0] &&
           a->center[1] == b->center[1] &&
           a->center[2] == b->center[2] &&
           a->radius == b->radius &&
           arr16_eq(a->view, b->view) &&
           arr16_eq(a->proj, b->proj) &&
           a->lit == b->lit;
}

// Sortier-Ordnung ueber den (fuer das Merging relevanten) Zustand.
// view/proj sind pro Frame konstant und werden daher nicht verglichen.
static int cmd_key_less(const DrawCmd *a, const DrawCmd *b) {
    if (a->mode != b->mode) return a->mode < b->mode;
    if (a->lit != b->lit) return a->lit < b->lit;
    if (a->effect != b->effect) return a->effect < b->effect;
    if (a->lineW != b->lineW) return a->lineW < b->lineW;
    if (a->pointSize != b->pointSize) return a->pointSize < b->pointSize;
    if (a->radius != b->radius) return a->radius < b->radius;
    if (a->center[0] != b->center[0]) return a->center[0] < b->center[0];
    if (a->center[1] != b->center[1]) return a->center[1] < b->center[1];
    if (a->center[2] != b->center[2]) return a->center[2] < b->center[2];
    if (a->col.r != b->col.r) return a->col.r < b->col.r;
    if (a->col.g != b->col.g) return a->col.g < b->col.g;
    if (a->col.b != b->col.b) return a->col.b < b->col.b;
    if (a->col.a != b->col.a) return a->col.a < b->col.a;
    if (a->grad2.r != b->grad2.r) return a->grad2.r < b->grad2.r;
    if (a->grad2.g != b->grad2.g) return a->grad2.g < b->grad2.g;
    if (a->grad2.b != b->grad2.b) return a->grad2.b < b->grad2.b;
    if (a->grad2.a != b->grad2.a) return a->grad2.a < b->grad2.a;
    return 0;
}

/* --- Sortier-Kontext fuer qsort (ein Thread, daher static ok) --- */
static DrawCmd *sort_cmds = NULL;      // aktuelle Befehlstabelle

static int cmd_order_cmp(const void *pa, const void *pb) {
    int a = *((const int *) pa);
    int b = *((const int *) pb);
    int la = cmd_key_less(&sort_cmds[a], &sort_cmds[b]);
    int lb = cmd_key_less(&sort_cmds[b], &sort_cmds[a]);
    if (la) return -1;
    if (lb) return 1;
    return 0;
}

// Haengt den Vertex-Block von src an dst an und legt (bei gleichem Zustand
// wie der letzte Befehl) die Kommandos zusammen.
static void merge_block(float *dstData, int *dstLen, DrawCmd *dstCmds,
                        int *dstCmdCount, DrawCmd src, const float *srcData) {
    DrawCmd c = src;
    int begin = c.first * 7;
    int end = begin + c.count * 7;
    c.first = *dstLen / 7;
    for (int i = begin; i < end; i++) {
        dstData[(*dstLen)++] = srcData[i];
    }
    if (*dstCmdCount > 0 && can_merge(&dstCmds[*dstCmdCount - 1], &c)) {
        dstCmds[*dstCmdCount - 1].count += c.count;
    } else {
        dstCmds[(*dstCmdCount)++] = c;
    }
}

/* ==================================================================
 * BATCHING - SAMMELN & ZEICHNEN (Kern des Renderers)
 * ================================================================== */

// Sammelt eine Immediate-Primitive im Frame-Batch. Die Modell-Matrix wird
// in die Vertices gebacken und die Farbe pro Vertex abgelegt.
static void submit(uint32_t mode, const float *verts, int nfloats,
                   int useStroke, const Mat4x4 *model) {
    ColorState col = state.fill;
    int lit = 1;   // gefuellte Flaechen werden beleuchtet
    if (useStroke) {
        col = state.stroke;
        lit = 0;
    }
    int flat = (state.effect == EFFECT_FLAT);

    // Flache Primitive tragen ihre Farbe pro Vertex (uColor bleibt weiss);
    // Gradient/Pulse nutzen uColor (Vertex-Farbe bleibt weiss).
    ColorState vertexCol = {1, 1, 1, 1};
    ColorState uniformCol = {1, 1, 1, 1};
    if (flat) vertexCol = col; else uniformCol = col;

    DrawCmd cmd = {0};
    cmd.mode = mode;
    cmd.first = batchVertCount / 7;
    cmd.count = nfloats / 3;
    cmd.col = uniformCol;
    cmd.effect = state.effect;
    cmd.grad2 = state.grad2;
    cmd.pointSize = pointSizeVal;
    cmd.lineW = state.lineW;
    cmd.center[0] = gradCenter[0];
    cmd.center[1] = gradCenter[1];
    cmd.center[2] = gradCenter[2];
    cmd.radius = gradRadius;
    memcpy(cmd.view, viewUniform, sizeof(cmd.view));
    memcpy(cmd.proj, projUniform, sizeof(cmd.proj));
    cmd.lit = lit;

    // Vertices mit der Modell-Matrix backen und mit Farbe interleaven.
    Mat4x4 mmat = model ? *model : mat4_identity();
    int n = nfloats / 3;
    for (int i = 0; i < n; i++) {
        double x = verts[i * 3 + 0];
        double y = verts[i * 3 + 1];
        double z = verts[i * 3 + 2];
        if (batchVertCount + 7 > (int) MAX_BATCH_VERTS) {
            fprintf(stderr, "lib3d: Batch-Vert-Puffer voll (%u)\n",
                    (unsigned) MAX_BATCH_VERTS);
            return;
        }
        batchVerts[batchVertCount++] = (float)(mmat.m[0][0] * x + mmat.m[0][1] * y +
                                               mmat.m[0][2] * z + mmat.m[0][3]);
        batchVerts[batchVertCount++] = (float)(mmat.m[1][0] * x + mmat.m[1][1] * y +
                                               mmat.m[1][2] * z + mmat.m[1][3]);
        batchVerts[batchVertCount++] = (float)(mmat.m[2][0] * x + mmat.m[2][1] * y +
                                               mmat.m[2][2] * z + mmat.m[2][3]);
        batchVerts[batchVertCount++] = (float)vertexCol.r;
        batchVerts[batchVertCount++] = (float)vertexCol.g;
        batchVerts[batchVertCount++] = (float)vertexCol.b;
        batchVerts[batchVertCount++] = (float)vertexCol.a;
    }

    // Aufeinanderfolgende Befehle mit identischem Zustand zusammenfassen.
    if (batchCmdCount > 0 && can_merge(&batchCmds[batchCmdCount - 1], &cmd)) {
        batchCmds[batchCmdCount - 1].count += cmd.count;
        return;
    }
    if (batchCmdCount >= (int) MAX_BATCH_CMDS) {
        fprintf(stderr, "lib3d: Batch-Cmd-Puffer voll (%u)\n",
                (unsigned) MAX_BATCH_CMDS);
        return;
    }
    batchCmds[batchCmdCount++] = cmd;
}

static void submit_triangles(const float *verts, int nfloats, const Mat4x4 *model) {
    submit(GL_TRIANGLES, verts, nfloats, 0, model);
}

static void submit_lines(const float *verts, int nfloats, const Mat4x4 *model) {
    submit(GL_LINES, verts, nfloats, 1, model);
}

// Sammelt einen GPU-Instancing-Zeichenauftrag.
static void submit_instanced(const float *triangles, int tri_n,
                             const float *lines, int line_n,
                             DrawStyle style, const Instance *instances,
                             int count) {
    if (count <= 0) return;

    // Instanz-Daten: Modell-Matrix (Spalten-major) + Farbe = 20 Floats.
    int instFirst = instDataCount / 20;
    for (int k = 0; k < count; k++) {
        const Mat4x4 *m = &instances[k].model;
        ColorState col = instances[k].color;
        if (instDataCount + 20 > (int) MAX_INST_DATA) {
            fprintf(stderr, "lib3d: Instanz-Puffer voll (%u)\n",
                    (unsigned) MAX_INST_DATA);
            return;
        }
        instData[instDataCount++] = (float)m->m[0][0];
        instData[instDataCount++] = (float)m->m[1][0];
        instData[instDataCount++] = (float)m->m[2][0];
        instData[instDataCount++] = (float)m->m[3][0];
        instData[instDataCount++] = (float)m->m[0][1];
        instData[instDataCount++] = (float)m->m[1][1];
        instData[instDataCount++] = (float)m->m[2][1];
        instData[instDataCount++] = (float)m->m[3][1];
        instData[instDataCount++] = (float)m->m[0][2];
        instData[instDataCount++] = (float)m->m[1][2];
        instData[instDataCount++] = (float)m->m[2][2];
        instData[instDataCount++] = (float)m->m[3][2];
        instData[instDataCount++] = (float)m->m[0][3];
        instData[instDataCount++] = (float)m->m[1][3];
        instData[instDataCount++] = (float)m->m[2][3];
        instData[instDataCount++] = (float)m->m[3][3];
        instData[instDataCount++] = (float)col.r;
        instData[instDataCount++] = (float)col.g;
        instData[instDataCount++] = (float)col.b;
        instData[instDataCount++] = (float)col.a;
    }
    int instCount = count;

    // Basis-Geometrie anhaengen (Farbe weiss; pro Instanz ueberschrieben).
    int nbTri = tri_n / 3;
    int nbLines = line_n / 3;

    if ((style == STYLE_FILL || style == STYLE_BOTH) && tri_n > 0) {
        InstCmd c = {0};
        c.mode = GL_TRIANGLES;
        c.first = instGeomCount / 7;
        c.count = nbTri;
        c.effect = state.effect;
        c.grad2 = state.grad2;
        c.pointSize = pointSizeVal;
        c.lineW = state.lineW;
        c.center[0] = gradCenter[0];
        c.center[1] = gradCenter[1];
        c.center[2] = gradCenter[2];
        c.radius = gradRadius;
        memcpy(c.view, viewUniform, sizeof(c.view));
        memcpy(c.proj, projUniform, sizeof(c.proj));
        c.lit = 1;
        if (instGeomCount + nbTri * 7 > (int) MAX_INST_GEOM) {
            fprintf(stderr, "lib3d: Instanz-Geometrie-Puffer voll (%u)\n",
                    (unsigned) MAX_INST_GEOM);
            return;
        }
        for (int k = 0; k < nbTri; k++) {
            instGeom[instGeomCount++] = triangles[k * 3 + 0];
            instGeom[instGeomCount++] = triangles[k * 3 + 1];
            instGeom[instGeomCount++] = triangles[k * 3 + 2];
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
        }
        c.instFirst = instFirst;
        c.instCount = instCount;
        if (instCmdCount < (int) MAX_INST_CMDS) instCmds[instCmdCount++] = c;
    }
    if ((style == STYLE_STROKE || style == STYLE_BOTH) && line_n > 0) {
        InstCmd c = {0};
        c.mode = GL_LINES;
        c.first = instGeomCount / 7;
        c.count = nbLines;
        c.effect = state.effect;
        c.grad2 = state.grad2;
        c.pointSize = pointSizeVal;
        c.lineW = state.lineW;
        c.center[0] = gradCenter[0];
        c.center[1] = gradCenter[1];
        c.center[2] = gradCenter[2];
        c.radius = gradRadius;
        memcpy(c.view, viewUniform, sizeof(c.view));
        memcpy(c.proj, projUniform, sizeof(c.proj));
        c.lit = 0;
        if (instGeomCount + nbLines * 7 > (int) MAX_INST_GEOM) {
            fprintf(stderr, "lib3d: Instanz-Geometrie-Puffer voll (%u)\n",
                    (unsigned) MAX_INST_GEOM);
            return;
        }
        for (int k = 0; k < nbLines; k++) {
            instGeom[instGeomCount++] = lines[k * 3 + 0];
            instGeom[instGeomCount++] = lines[k * 3 + 1];
            instGeom[instGeomCount++] = lines[k * 3 + 2];
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
            instGeom[instGeomCount++] = 1.0f;
        }
        c.instFirst = instFirst;
        c.instCount = instCount;
        if (instCmdCount < (int) MAX_INST_CMDS) instCmds[instCmdCount++] = c;
    }
}

// Zeichnet alle gesammelten Instancing-Auftraege.
static void flush_instanced(void) {
    if (instCmdCount == 0) return;

    glBindVertexArray(vao);

    // Basis-Geometrie hochladen.
    glBindBuffer(GL_ARRAY_BUFFER, vboGeom);
    int geomBytes = instGeomCount * (int) sizeof(float);
    if (geomBytes > geomCapBytes) {
        glBufferData(GL_ARRAY_BUFFER, geomBytes, instGeom, GL_DYNAMIC_DRAW);
        geomCapBytes = geomBytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, geomBytes, instGeom);
    }
    GLsizei gStride = 7 * 4;
    glEnableVertexAttribArray((GLuint) locPos);
    glEnableVertexAttribArray((GLuint) locColorAttr);
    glVertexAttribPointer((GLuint) locPos, 3, GL_FLOAT, GL_FALSE, gStride, NULL);
    glVertexAttribPointer((GLuint) locColorAttr, 4, GL_FLOAT, GL_FALSE, gStride,
                          (const void *) (3 * 4));
    glVertexAttribDivisor((GLuint) locPos, 0);
    glVertexAttribDivisor((GLuint) locColorAttr, 0);

    // Instanz-Daten hochladen (20 floats/Instanz).
    glBindBuffer(GL_ARRAY_BUFFER, vboInst);
    int instBytes = instDataCount * (int) sizeof(float);
    if (instBytes > instCapBytes) {
        glBufferData(GL_ARRAY_BUFFER, instBytes, instData, GL_DYNAMIC_DRAW);
        instCapBytes = instBytes;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, instBytes, instData);
    }
    GLsizei iStride = 20 * 4;
    for (int k = 0; k < 4; k++) {
        GLuint loc = (GLuint)(locInstModel + k);
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, iStride,
                              (const void *) (size_t) (k * 4 * 4));
        glVertexAttribDivisor(loc, 1);
    }
    glEnableVertexAttribArray((GLuint) locInstColor);
    glVertexAttribPointer((GLuint) locInstColor, 4, GL_FLOAT, GL_FALSE, iStride,
                          (const void *) (16 * 4));
    glVertexAttribDivisor((GLuint) locInstColor, 1);

    glUniform1i(locInstanced, 1);
    glUniform4f(locColor, 1, 1, 1, 1);
    for (int i = 0; i < instCmdCount; i++) {
        InstCmd *c = &instCmds[i];
        // GL 3.3 kennt keinen baseInstance-Parameter - die Instanz-Attribute
        // werden pro Kommando auf den Start der jeweiligen Instanzen gesetzt.
        int instOffset = c->instFirst * 20 * 4;
        for (int k = 0; k < 4; k++) {
            GLuint loc = (GLuint)(locInstModel + k);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, iStride,
                                  (const void *) (size_t) (instOffset + k * 4 * 4));
        }
        glVertexAttribPointer((GLuint) locInstColor, 4, GL_FLOAT, GL_FALSE, iStride,
                              (const void *) (size_t) (instOffset + 16 * 4));

        glUniformMatrix4fv(locView, 1, GL_FALSE, c->view);
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, c->proj);
        glUniform1i(locMode, effect_to_mode(c->effect));
        glUniform1i(locLighted, c->lit);
        glUniform4f(locColor2, (float)c->grad2.r, (float)c->grad2.g,
                    (float)c->grad2.b, (float)c->grad2.a);
        glUniform1f(locPointSize, (float)c->pointSize);
        glUniform3f(locCenter, (float)c->center[0], (float)c->center[1],
                    (float)c->center[2]);
        glUniform1f(locRadius, (float)c->radius);
        glLineWidth((float)c->lineW);
        glDrawArraysInstanced(c->mode, c->first, c->count, c->instCount);
    }
    glUniform1i(locInstanced, 0);

    // Divisoren/Attribute zuruecksetzen, damit normale Draws sauber bleiben.
    for (int k = 0; k < 4; k++) {
        GLuint loc = (GLuint)(locInstModel + k);
        glVertexAttribDivisor(loc, 0);
        glDisableVertexAttribArray(loc);
    }
    glVertexAttribDivisor((GLuint) locInstColor, 0);
    glDisableVertexAttribArray((GLuint) locInstColor);

    instGeomCount = 0;
    instDataCount = 0;
    instCmdCount = 0;
}

// Zeichnet alle gesammelten Primitives des Frames mit einem einzigen
// wiederverwendbaren VBO.
void l3d_flush_batch(void) {
    if (batchCmdCount == 0 && instCmdCount == 0) return;

    l3d_compact_batch();

    glUniform1i(locInstanced, 0);

    if (batchCmdCount > 0) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        int byteLen = batchVertCount * (int) sizeof(float);
        if (byteLen > batchCapBytes) {
            glBufferData(GL_ARRAY_BUFFER, byteLen, batchVerts, GL_DYNAMIC_DRAW);
            batchCapBytes = byteLen;
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, byteLen, batchVerts);
        }

        glEnableVertexAttribArray((GLuint) locPos);
        glEnableVertexAttribArray((GLuint) locColorAttr);
        GLsizei stride = 7 * 4;
        glVertexAttribPointer((GLuint) locPos, 3, GL_FLOAT, GL_FALSE, stride, NULL);
        glVertexAttribPointer((GLuint) locColorAttr, 4, GL_FLOAT, GL_FALSE, stride,
                              (const void *) (3 * 4));

        for (int i = 0; i < batchCmdCount; i++) {
            DrawCmd *c = &batchCmds[i];
            glUniformMatrix4fv(locView, 1, GL_FALSE, c->view);
            glUniformMatrix4fv(locProjection, 1, GL_FALSE, c->proj);
            glUniform1i(locMode, effect_to_mode(c->effect));
            glUniform1i(locLighted, c->lit);
            glUniform4f(locColor, (float)c->col.r, (float)c->col.g,
                        (float)c->col.b, (float)c->col.a);
            glUniform4f(locColor2, (float)c->grad2.r, (float)c->grad2.g,
                        (float)c->grad2.b, (float)c->grad2.a);
            glUniform1f(locPointSize, (float)c->pointSize);
            glUniform3f(locCenter, (float)c->center[0], (float)c->center[1],
                        (float)c->center[2]);
            glUniform1f(locRadius, (float)c->radius);
            glLineWidth((float)c->lineW);
            glDrawArrays(c->mode, c->first, c->count);
        }

        batchVertCount = 0;
        batchCmdCount = 0;
    }

    flush_instanced();
}

// Sortiert (falls aktiviert) und fasst die Pending-Batch zusammen.
void l3d_compact_batch(void) {
    if (!batchSort || batchCmdCount < 2) return;

    static int order[MAX_BATCH_CMDS];
    static float scratchVerts[MAX_BATCH_VERTS];
    static DrawCmd scratchCmds[MAX_BATCH_CMDS];

    for (int i = 0; i < batchCmdCount; i++) order[i] = i;

    sort_cmds = batchCmds;
    qsort(order, (size_t) batchCmdCount, sizeof(int), cmd_order_cmp);
    sort_cmds = NULL;

    int scratchLen = 0;
    int scratchCmdCount = 0;
    for (int i = 0; i < batchCmdCount; i++) {
        merge_block(scratchVerts, &scratchLen, scratchCmds, &scratchCmdCount,
                    batchCmds[order[i]], batchVerts);
    }

    memcpy(batchVerts, scratchVerts, (size_t) scratchLen * sizeof(float));
    batchVertCount = scratchLen;
    memcpy(batchCmds, scratchCmds, (size_t) scratchCmdCount * sizeof(DrawCmd));
    batchCmdCount = scratchCmdCount;
}

/* ==================================================================
 * MAUS / FENSTER (intern)
 * ================================================================== */

static void on_cursor_pos(GLFWwindow *win, double x, double y) {
    (void) win;
    mouseX = x - (double) frameW / 2.0;
    mouseY = -((y - (double) frameH / 2.0));
}

static void on_mouse_button(GLFWwindow *win, int button, int action, int mods) {
    (void) win; (void) mods;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS)   mouseStatus = 1;
    if (action == GLFW_RELEASE) mouseStatus = 2;
}

static void on_key(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void) win; (void) scancode; (void) mods;
    if (key >= 0 && key < K_MAX_KEYS) {
        if (action == GLFW_PRESS) {
            keyDown[key] = 1;
            keyPressed[key] = 1; // Kante merken (GLFW_REPEAT zaehlt nicht)
        } else if (action == GLFW_RELEASE) {
            keyDown[key] = 0;
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_F11) {
        toggleFullscreenWanted = 1;
    }
}

static void on_framebuffer_size(GLFWwindow *win, int width, int height) {
    (void) win;
    frameW = width;
    frameH = height;
    glViewport(0, 0, width, height);
}

/* ==================================================================
 * OEFFENTLICHE API - Fenster & Eingabe
 * ================================================================== */

int l3d_get_width(void)  { return frameW; }
int l3d_get_height(void) { return frameH; }

void l3d_no_loop(void) { looping = 0; }

int l3d_mouse_down(void) { return mouseStatus == 1; }

int l3d_mouse_up(void) {
    if (mouseStatus == 2) {
        mouseStatus = 0;
        return 1;
    }
    return 0;
}

int l3d_is_fullscreen(void) { return fullscreen; }

void l3d_set_fullscreen(int on) {
    if (window == NULL || (on ? 1 : 0) == fullscreen) return;
    if (on) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (!monitor) return; // ohne Monitor kein Vollbild
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (!mode) return;
        glfwGetWindowPos(window, &windowedPos[0], &windowedPos[1]);
        glfwGetWindowSize(window, &windowedSize[0], &windowedSize[1]);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, NULL, windowedPos[0], windowedPos[1],
                             windowedSize[0], windowedSize[1], 0);
    }
    fullscreen = on;

    // Framebuffer-Groesse sofort uebernehmen (Callback feuert ggf. erst
    // beim naechsten Poll).
    glfwGetFramebufferSize(window, &frameW, &frameH);
    glViewport(0, 0, frameW, frameH);
}

void l3d_toggle_fullscreen(void) { l3d_set_fullscreen(!fullscreen); }

int l3d_key_down(int key) {
    if (key < 0 || key >= K_MAX_KEYS) return 0;
    return keyDown[key];
}

int l3d_key_pressed(int key) {
    if (key < 0 || key >= K_MAX_KEYS) return 0;
    if (keyPressed[key]) {
        keyPressed[key] = 0;
        return 1;
    }
    return 0;
}

/* ==================================================================
 * OEFFENTLICHE API - Matrizen & Szene
 * ================================================================== */

void l3d_set_projection(const Mat4x4 *m) {
    flatten_matrix(m, projUniform);
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, projUniform);
}

void l3d_set_view(const Mat4x4 *m) {
    flatten_matrix(m, viewUniform);
    glUniformMatrix4fv(locView, 1, GL_FALSE, viewUniform);
}

void l3d_apply_camera(Camera cam) {
    double aspect = frameH > 0 ? (double) frameW / (double) frameH : 1.0;
    Mat4x4 proj = camera_projection(cam, aspect);
    Mat4x4 view = camera_view(cam);
    l3d_set_projection(&proj);
    l3d_set_view(&view);
}

void l3d_begin_scene(Scene scene) {
    l3d_background(scene.background);
    if (scene.fog.enabled) {
        ColorState c = scene.fog.color;
        l3d_set_fog(scene.fog.near, scene.fog.far, c.r, c.g, c.b, c.a);
    }
    l3d_apply_camera(scene.cam);
    Mat4x4 view = camera_view(scene.cam);
    Vec3 camLight = vec3_transform_dir(scene.sunDir, &view);
    l3d_set_light_direction(camLight.x, camLight.y, camLight.z);
}

int l3d_pending_draw_calls(void)           { return batchCmdCount; }
int l3d_pending_instanced_draw_calls(void) { return instCmdCount; }
void l3d_set_batch_sort(int on)            { batchSort = on ? 1 : 0; }

void l3d_set_gradient_center(double cx, double cy, double cz, double radius) {
    gradCenter[0] = cx; gradCenter[1] = cy; gradCenter[2] = cz;
    gradRadius = radius;
    glUniform3f(locCenter, (float)cx, (float)cy, (float)cz);
    glUniform1f(locRadius, (float)radius);
}

void l3d_set_fog(double near, double far, double r, double g, double b, double a) {
    glUniform1f(locFogNear, (float)near);
    glUniform1f(locFogFar, (float)far);
    glUniform4f(locFogColor, (float)r, (float)g, (float)b, (float)a);
}

void l3d_set_light_direction(double x, double y, double z) {
    glUniform3f(locLightDir, (float)x, (float)y, (float)z);
}

/* ==================================================================
 * OEFFENTLICHE API - Zeichenzustand
 * ================================================================== */

void l3d_fill_color(ColorState color)        { state.fill = color; }
void l3d_fill_color_input(ColorInput color)  { l3d_fill_color(color.color); }
void l3d_stroke_color(ColorState color)      { state.stroke = color; }
void l3d_stroke_color_input(ColorInput color){ l3d_stroke_color(color.color); }
void l3d_stroke_width(double w)              { state.lineW = w; }
void l3d_set_effect(EffectMode effect)       { state.effect = effect; }
void l3d_set_gradient(ColorState color)      { state.grad2 = color; }
void l3d_set_gradient_input(ColorInput color){ l3d_set_gradient(color.color); }

void l3d_background(ColorState color) {
    l3d_flush_batch();
    glClearColor((float)color.r, (float)color.g, (float)color.b, (float)color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void l3d_background_input(ColorInput color)  { l3d_background(color.color); }

/* ==================================================================
 * ZEICHEN-FASSADE (Port von draw.go)
 * ================================================================== */

static void apply_style(ColorInput stroke, ColorInput fill,
                        double lineWidth, double pointSize) {
    if (stroke.set)  l3d_stroke_color(stroke.color);
    if (fill.set)    l3d_fill_color(fill.color);
    if (lineWidth != 0) l3d_stroke_width(lineWidth);
    if (pointSize != 0) l3d_point_size(pointSize);
}

void l3d_draw_mesh(Mesh mesh, ShapeStyle style, const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    if (style.mode == STYLE_FILL || style.mode == STYLE_BOTH) {
        submit_triangles(mesh.triangles, mesh.tri_floats, model);
    }
    if (style.mode == STYLE_STROKE || style.mode == STYLE_BOTH) {
        submit_lines(mesh.lines, mesh.line_floats, model);
    }
}

void l3d_draw_mesh_instanced(Mesh mesh, ShapeStyle style,
                             const Instance *instances, int count) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    submit_instanced(mesh.triangles, mesh.tri_floats, mesh.lines,
                     mesh.line_floats, style.mode, instances, count);
}

void l3d_draw_point(double x, double y, double z, PrimitiveStyle style,
                    const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_point(x, y, z, model);
}

void l3d_draw_line(double x1, double y1, double z1, double x2, double y2,
                   double z2, PrimitiveStyle style, const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_line(x1, y1, z1, x2, y2, z2, model);
}

void l3d_draw_triangle(double x1, double y1, double z1, double x2, double y2,
                       double z2, double x3, double y3, double z3,
                       ShapeStyle style, const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_triangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, style.mode, model);
}

void l3d_draw_shape(double x1, double y1, double z1, double x2, double y2,
                    double z2, double x3, double y3, double z3, double x4,
                    double y4, double z4, ShapeStyle style, const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_shape(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, style.mode, model);
}

void l3d_draw_rect(double x, double y, double w, double h, ShapeStyle style,
                   const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_rect(x, y, w, h, style.mode, style.z, model);
}

void l3d_draw_circle(double x, double y, double z, double radius, ShapeStyle style,
                     const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    int segments = style.segments;
    if (segments == 0) segments = 64;
    l3d_circle(x, y, z, radius, style.mode, segments, model);
}

void l3d_draw_polygon(const double *pts, int nfloats, ShapeStyle style,
                      const Mat4x4 *model) {
    apply_style(style.stroke, style.fill, style.lineWidth, style.pointSize);
    l3d_polygon(pts, nfloats, style.mode, model);
}

/* ==================================================================
 * IMMEDIATE-PRIMITIVE (Port von renderer.go)
 * ================================================================== */

void l3d_point_size(double px) {
    pointSizeVal = px;
    glUniform1f(locPointSize, (float)px);
}

void l3d_point(double x, double y, double z, const Mat4x4 *model) {
    float v[3] = {(float)x, (float)y, (float)z};
    submit(GL_POINTS, v, 3, 1, model);
}

void l3d_line(double x1, double y1, double z1, double x2, double y2,
              double z2, const Mat4x4 *model) {
    float v[6] = {(float)x1, (float)y1, (float)z1, (float)x2, (float)y2, (float)z2};
    submit(GL_LINES, v, 6, 1, model);
}

void l3d_triangle(double x1, double y1, double z1, double x2, double y2,
                  double z2, double x3, double y3, double z3, DrawStyle style,
                  const Mat4x4 *model) {
    float pts[9] = {(float)x1, (float)y1, (float)z1,
                    (float)x2, (float)y2, (float)z2,
                    (float)x3, (float)y3, (float)z3};
    if (style == STYLE_FILL || style == STYLE_BOTH) {
        submit(GL_TRIANGLES, pts, 9, 0, model);
    }
    if (style == STYLE_STROKE || style == STYLE_BOTH) {
        float s[18] = {(float)x1, (float)y1, (float)z1, (float)x2, (float)y2, (float)z2,
                       (float)x2, (float)y2, (float)z2, (float)x3, (float)y3, (float)z3,
                       (float)x3, (float)y3, (float)z3, (float)x1, (float)y1, (float)z1};
        submit(GL_LINES, s, 18, 1, model);
    }
}

void l3d_shape(double x1, double y1, double z1, double x2, double y2, double z2,
               double x3, double y3, double z3, double x4, double y4, double z4,
               DrawStyle style, const Mat4x4 *model) {
    if (style == STYLE_FILL || style == STYLE_BOTH) {
        float f[18] = {(float)x1, (float)y1, (float)z1, (float)x2, (float)y2, (float)z2,
                       (float)x3, (float)y3, (float)z3,
                       (float)x1, (float)y1, (float)z1, (float)x3, (float)y3, (float)z3,
                       (float)x4, (float)y4, (float)z4};
        submit(GL_TRIANGLES, f, 18, 0, model);
    }
    if (style == STYLE_STROKE || style == STYLE_BOTH) {
        float s[24] = {(float)x1, (float)y1, (float)z1, (float)x2, (float)y2, (float)z2,
                       (float)x2, (float)y2, (float)z2, (float)x3, (float)y3, (float)z3,
                       (float)x3, (float)y3, (float)z3, (float)x4, (float)y4, (float)z4,
                       (float)x4, (float)y4, (float)z4, (float)x1, (float)y1, (float)z1};
        submit(GL_LINES, s, 24, 1, model);
    }
}

void l3d_rect(double x, double y, double w, double h, DrawStyle style, double z,
              const Mat4x4 *model) {
    double hw = w / 2.0, hh = h / 2.0;
    l3d_shape(x - hw, y - hh, z, x + hw, y - hh, z, x + hw, y + hh, z,
              x - hw, y + hh, z, style, model);
}

void l3d_circle(double x, double y, double z, double radius, DrawStyle style,
                int segments, const Mat4x4 *model) {
    double tau = 2.0 * M_PI;
    if (segments < 3) segments = 3;

    if (style == STYLE_FILL || style == STYLE_BOTH) {
        float *fillVerts = malloc((size_t) segments * 9 * sizeof(float));
        int fi = 0;
        for (int i = 0; i < segments; i++) {
            double a0 = ((double) i / segments) * tau;
            double a1 = ((double) (i + 1) / segments) * tau;
            fillVerts[fi++] = (float)x;
            fillVerts[fi++] = (float)y;
            fillVerts[fi++] = (float)z;
            fillVerts[fi++] = (float)(x + cos(a0) * radius);
            fillVerts[fi++] = (float)(y + sin(a0) * radius);
            fillVerts[fi++] = (float)z;
            fillVerts[fi++] = (float)(x + cos(a1) * radius);
            fillVerts[fi++] = (float)(y + sin(a1) * radius);
            fillVerts[fi++] = (float)z;
        }
        submit(GL_TRIANGLES, fillVerts, segments * 9, 0, model);
        free(fillVerts);
    }
    if (style == STYLE_STROKE || style == STYLE_BOTH) {
        float *strokeVerts = malloc((size_t) segments * 3 * sizeof(float));
        int si = 0;
        for (int i = 0; i < segments; i++) {
            double a = ((double) i / segments) * tau;
            strokeVerts[si++] = (float)(x + cos(a) * radius);
            strokeVerts[si++] = (float)(y + sin(a) * radius);
            strokeVerts[si++] = (float)z;
        }
        submit(GL_LINE_LOOP, strokeVerts, segments * 3, 1, model);
        free(strokeVerts);
    }
}

void l3d_polygon(const double *pts, int nfloats, DrawStyle style,
                 const Mat4x4 *model) {
    if (nfloats < 4) return;
    int n = nfloats / 3;

    if (style == STYLE_FILL || style == STYLE_BOTH) {
        float *fillVerts = malloc((size_t)(n - 2) * 9 * sizeof(float));
        int fi = 0;
        for (int i = 1; i < n - 1; i++) {
            fillVerts[fi++] = (float)pts[0];
            fillVerts[fi++] = (float)pts[1];
            fillVerts[fi++] = (float)pts[2];
            fillVerts[fi++] = (float)pts[i * 3];
            fillVerts[fi++] = (float)pts[i * 3 + 1];
            fillVerts[fi++] = (float)pts[i * 3 + 2];
            fillVerts[fi++] = (float)pts[i * 3 + 3];
            fillVerts[fi++] = (float)pts[i * 3 + 4];
            fillVerts[fi++] = (float)pts[i * 3 + 5];
        }
        submit(GL_TRIANGLES, fillVerts, (n - 2) * 9, 0, model);
        free(fillVerts);
    }
    if (style == STYLE_STROKE || style == STYLE_BOTH) {
        float *strokeVerts = malloc((size_t) nfloats * sizeof(float));
        for (int i = 0; i < nfloats; i++) strokeVerts[i] = (float) pts[i];
        submit(GL_LINE_LOOP, strokeVerts, nfloats, 1, model);
        free(strokeVerts);
    }
}

/* ==================================================================
 * KAMERA- UND SZENEN-BUENDEL
 * ================================================================== */

Mat4x4 camera_view(Camera c) {
    return mat4_lookat(c.pos, c.target, c.up);
}

Mat4x4 camera_projection(Camera c, double aspect) {
    return mat4_perspective(c.fovY, aspect, c.near, c.far);
}

Scene scene_new(void) {
    Scene s;
    s.cam.pos    = vec3(50, 100, 200);
    s.cam.target = vec3(0, 0, 0);
    s.cam.up     = vec3(0, 1, 0);
    s.cam.fovY   = 1.2;
    s.cam.near   = 0.1;
    s.cam.far    = 1000;
    s.sunDir     = vec3(0.5, 1.0, 0.3);
    s.fog.enabled = 1;
    s.fog.near    = 100;
    s.fog.far     = 400;
    s.fog.color   = (ColorState){0.25, 0.25, 0.25, 1};
    s.background  = (ColorState){40 / 255.0, 40 / 255.0, 40 / 255.0, 1};
    return s;
}

/* ==================================================================
 * SHADER-HILFEN  (Port von shader.go)
 * ================================================================== */

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    const GLchar *csrc = (const GLchar *) src;
    glShaderSource(shader, 1, &csrc, NULL);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        log[1023] = '\0';
        fprintf(stderr, "lib3d: Shader-Fehler: %s\n", log);
    }
    return shader;
}

static GLuint create_program(const char *vs, const char *fs) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vs);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, fs);

    GLuint p = glCreateProgram();
    glAttachShader(p, vert);
    glAttachShader(p, frag);
    glLinkProgram(p);

    GLint status = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        log[1023] = '\0';
        fprintf(stderr, "lib3d: Programm-Fehler: %s\n", log);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return p;
}

/* ==================================================================
 * SETUP & ANIMATIONS-SCHLEIFE (Port von renderer.go Init/StartAnimation)
 * ================================================================== */

static void default_window_size(int *w, int *h) {
    const int wantW = 1280, wantH = 800;
    int ww = wantW, hh = wantH;
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (mode) {
            if (mode->width - 100 < ww) ww = mode->width - 100;
            if (mode->height - 100 < hh) hh = mode->height - 100;
        }
    }
    if (ww < 320) ww = 320;
    if (hh < 240) hh = 240;
    *w = ww;
    *h = hh;
}

int l3d_init(void) {
    if (!glfwInit()) {
        fprintf(stderr, "lib3d: GLFW-Init fehlgeschlagen\n");
        return 0;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    int winW = 0, winH = 0;
    default_window_size(&winW, &winH);

    window = glfwCreateWindow(winW, winH, "3D Renderer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "lib3d: Fenster-Erstellung fehlgeschlagen\n");
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (mode) glfwSetWindowPos(window, (mode->width - winW) / 2,
                                   (mode->height - winH) / 2);
    }

    if (!l3d_gl_load()) {
        fprintf(stderr, "lib3d: OpenGL-Funktionen konnten nicht geladen werden\n");
        glfwTerminate();
        return 0;
    }

    program = create_program(VERT_SRC, FRAG_SRC);
    if (!program) {
        glfwTerminate();
        return 0;
    }
    glUseProgram(program);

    locPos        = glGetAttribLocation(program, "aPos");
    locColorAttr  = glGetAttribLocation(program, "aColor");
    locInstModel  = glGetAttribLocation(program, "aInstModel");
    locInstColor  = glGetAttribLocation(program, "aInstColor");
    locInstanced  = glGetUniformLocation(program, "uInstanced");
    locView       = glGetUniformLocation(program, "uView");
    locProjection = glGetUniformLocation(program, "uProjection");
    locPointSize  = glGetUniformLocation(program, "uPointSize");
    locMode       = glGetUniformLocation(program, "uMode");
    locColor      = glGetUniformLocation(program, "uColor");
    locColor2     = glGetUniformLocation(program, "uColor2");
    locTime       = glGetUniformLocation(program, "uTime");
    locCenter     = glGetUniformLocation(program, "uShapeCenter");
    locRadius     = glGetUniformLocation(program, "uShapeRadius");
    locFogNear    = glGetUniformLocation(program, "uFogNear");
    locFogFar     = glGetUniformLocation(program, "uFogFar");
    locFogColor   = glGetUniformLocation(program, "uFogColor");
    locLighted    = glGetUniformLocation(program, "uLighted");
    locLightDir   = glGetUniformLocation(program, "uLightDir");

    // VAO/VBO anlegen (OpenGL 3.3 Core benoetigt ein VAO).
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &vboGeom);
    glGenBuffers(1, &vboInst);

    glUniform1f(locPointSize, 4.0f);
    glUniform3f(locCenter, 0, 0, 0);
    glUniform1f(locRadius, 1);
    glUniform1i(locLighted, 0);
    glUniform1i(locInstanced, 0);
    glUniform3f(locLightDir, 0, 0, -1); // Headlight: Kamera blickt +Z

    memcpy(projUniform, identity16, sizeof(projUniform));
    memcpy(viewUniform, identity16, sizeof(viewUniform));
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, projUniform);
    glUniformMatrix4fv(locView, 1, GL_FALSE, viewUniform);

    // Fog-Defaults (kein Nebel).
    glUniform1f(locFogNear, 0);
    glUniform1f(locFogFar, 1);
    glUniform4f(locFogColor, 0, 0, 0, 0);

    // Depth-Testing + Alpha-Blending.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glfwGetFramebufferSize(window, &frameW, &frameH);
    if (frameW <= 0 || frameH <= 0) {
        glfwTerminate();
        return 0;
    }
    glViewport(0, 0, frameW, frameH);
    startTime = glfwGetTime();

    // Zustand zuruecksetzen.
    state.fill   = (ColorState){1, 1, 1, 1};
    state.stroke = (ColorState){0, 0, 0, 1};
    state.lineW  = 1.0;
    state.effect = EFFECT_FLAT;
    state.grad2  = (ColorState){0, 0, 0, 1};
    memcpy(viewUniform, identity16, sizeof(viewUniform));
    memcpy(projUniform, identity16, sizeof(projUniform));
    batchSort = 0;
    geomCapBytes = 0;
    instCapBytes = 0;
    batchCapBytes = 0;
    batchVertCount = 0;
    batchCmdCount = 0;
    instGeomCount = 0;
    instDataCount = 0;
    instCmdCount = 0;
    pointSizeVal = 4.0;
    gradCenter[0] = gradCenter[1] = gradCenter[2] = 0.0;
    gradRadius = 1.0;

    memset(keyDown, 0, sizeof(keyDown));
    memset(keyPressed, 0, sizeof(keyPressed));

    fullscreen = 0;
    toggleFullscreenWanted = 0;

    glfwSetCursorPosCallback(window, on_cursor_pos);
    glfwSetMouseButtonCallback(window, on_mouse_button);
    glfwSetKeyCallback(window, on_key);
    glfwSetFramebufferSizeCallback(window, on_framebuffer_size);

    return 1;
}

void l3d_start_animation(void (*fn)(void)) {
    looping = 1;
    while (looping && !glfwWindowShouldClose(window)) {
        double t = glfwGetTime() - startTime;
        glUniform1f(locTime, (float) t);

        fn();

        l3d_flush_batch();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // F11-Umschaltung (nach PollEvents anwenden).
        if (toggleFullscreenWanted) {
            toggleFullscreenWanted = 0;
            l3d_toggle_fullscreen();
        }
    }
    glfwTerminate();
}

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
