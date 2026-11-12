#ifndef GLCORE_H
#define GLCORE_H

// Minimaler OpenGL-3.3-Core-Loader (Ersatz fuer GLAD/GLEW).
//
// Deklariert die GL-Datentypen, die im Projekt benoetigten Konstanten
// und Funktionszeiger. Die Zeiger werden zur Laufzeit ueber
// glfwGetProcAddress geladen (siehe glcore.c). GLFW selbst bleibt
// funktionslos bzgl. GL - es wird GLFW_INCLUDE_NONE gesetzt.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- GL-Datentypen ---
typedef unsigned int  GLenum;
typedef unsigned char GLboolean;
typedef unsigned int  GLbitfield;
typedef signed char   GLbyte;
typedef short         GLshort;
typedef int           GLint;
typedef int           GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int  GLuint;
typedef float         GLfloat;
typedef float         GLclampf;
typedef double        GLdouble;
typedef char          GLchar;
typedef ptrdiff_t     GLintptr;
typedef ptrdiff_t     GLsizeiptr;

// --- Konstanten (nur die benoetigten) ---
#define GL_FALSE                0
#define GL_TRUE                 1
#define GL_POINTS               0x0000
#define GL_LINES                0x0001
#define GL_LINE_LOOP            0x0002
#define GL_TRIANGLES            0x0004
#define GL_VERTEX_SHADER        0x8B31
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_COMPILE_STATUS       0x8B81
#define GL_LINK_STATUS          0x8B82
#define GL_ARRAY_BUFFER         0x8892
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_FLOAT                0x1406
#define GL_DEPTH_TEST           0x0B71
#define GL_LEQUAL               0x0203
#define GL_BLEND                0x0BE2
#define GL_SRC_ALPHA            0x0302
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_PROGRAM_POINT_SIZE   0x8642
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_DEPTH_BUFFER_BIT     0x00000100

// --- Funktionszeiger ---
typedef GLuint   (*PFN_glCreateShader)(GLenum type);
typedef void     (*PFN_glShaderSource)(GLuint shader, GLsizei count,
                                       const GLchar **string, const GLint *length);
typedef void     (*PFN_glCompileShader)(GLuint shader);
typedef void     (*PFN_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
typedef void     (*PFN_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize,
                                          GLsizei *length, GLchar *infoLog);
typedef GLuint   (*PFN_glCreateProgram)(void);
typedef void     (*PFN_glAttachShader)(GLuint program, GLuint shader);
typedef void     (*PFN_glLinkProgram)(GLuint program);
typedef void     (*PFN_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
typedef void     (*PFN_glGetProgramInfoLog)(GLuint program, GLsizei bufSize,
                                            GLsizei *length, GLchar *infoLog);
typedef void     (*PFN_glDeleteShader)(GLuint shader);
typedef void     (*PFN_glUseProgram)(GLuint program);
typedef GLint    (*PFN_glGetAttribLocation)(GLuint program, const GLchar *name);
typedef GLint    (*PFN_glGetUniformLocation)(GLuint program, const GLchar *name);
typedef void     (*PFN_glUniform1i)(GLint location, GLint v0);
typedef void     (*PFN_glUniform1f)(GLint location, GLfloat v0);
typedef void     (*PFN_glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void     (*PFN_glUniform4f)(GLint location, GLfloat v0, GLfloat v1,
                                    GLfloat v2, GLfloat v3);
typedef void     (*PFN_glUniformMatrix4fv)(GLint location, GLsizei count,
                                           GLboolean transpose, const GLfloat *value);
typedef void     (*PFN_glGenVertexArrays)(GLsizei n, GLuint *arrays);
typedef void     (*PFN_glBindVertexArray)(GLuint array);
typedef void     (*PFN_glEnableVertexAttribArray)(GLuint index);
typedef void     (*PFN_glDisableVertexAttribArray)(GLuint index);
typedef void     (*PFN_glVertexAttribPointer)(GLuint index, GLint size, GLenum type,
                                             GLboolean normalized, GLsizei stride,
                                             const void *pointer);
typedef void     (*PFN_glVertexAttribDivisor)(GLuint index, GLuint divisor);
typedef void     (*PFN_glGenBuffers)(GLsizei n, GLuint *buffers);
typedef void     (*PFN_glBindBuffer)(GLenum target, GLuint buffer);
typedef void     (*PFN_glBufferData)(GLenum target, GLsizeiptr size, const void *data,
                                     GLenum usage);
typedef void     (*PFN_glBufferSubData)(GLenum target, GLintptr offset,
                                        GLsizeiptr size, const void *data);
typedef void     (*PFN_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
typedef void     (*PFN_glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count,
                                              GLsizei instancecount);
typedef void     (*PFN_glLineWidth)(GLfloat width);
typedef void     (*PFN_glEnable)(GLenum cap);
typedef void     (*PFN_glDepthFunc)(GLenum func);
typedef void     (*PFN_glBlendFunc)(GLenum sfactor, GLenum dfactor);
typedef void     (*PFN_glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void     (*PFN_glClear)(GLbitfield mask);
typedef void     (*PFN_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

#define L3D_GL_FUNCS(X) \
    X(glCreateShader) X(glShaderSource) X(glCompileShader) \
    X(glGetShaderiv) X(glGetShaderInfoLog) \
    X(glCreateProgram) X(glAttachShader) X(glLinkProgram) \
    X(glGetProgramiv) X(glGetProgramInfoLog) X(glDeleteShader) \
    X(glUseProgram) X(glGetAttribLocation) X(glGetUniformLocation) \
    X(glUniform1i) X(glUniform1f) X(glUniform3f) X(glUniform4f) \
    X(glUniformMatrix4fv) \
    X(glGenVertexArrays) X(glBindVertexArray) \
    X(glEnableVertexAttribArray) X(glDisableVertexAttribArray) \
    X(glVertexAttribPointer) X(glVertexAttribDivisor) \
    X(glGenBuffers) X(glBindBuffer) X(glBufferData) X(glBufferSubData) \
    X(glDrawArrays) X(glDrawArraysInstanced) X(glLineWidth) \
    X(glEnable) X(glDepthFunc) X(glBlendFunc) \
    X(glClearColor) X(glClear) X(glViewport)

#define L3D_GL_DECL(name) extern PFN_##name name;
L3D_GL_FUNCS(L3D_GL_DECL)
#undef L3D_GL_DECL

// Laedt alle Funktionszeiger; liefert 1 bei Erfolg, sonst 0.
int l3d_gl_load(void);

#ifdef __cplusplus
}
#endif

#endif // GLCORE_H
