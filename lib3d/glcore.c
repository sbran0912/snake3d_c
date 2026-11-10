#include "glcore.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

/* ==================================================================
 * Funktionszeiger-Definitionen + Loader (glfwGetProcAddress).
 * ================================================================== */

#define L3D_GL_DEFN(name) PFN_##name name = NULL;
L3D_GL_FUNCS(L3D_GL_DEFN)
#undef L3D_GL_DEFN

int l3d_gl_load(void) {
#define L3D_GL_LOAD(name)                      \
    do {                                       \
        name = (PFN_##name) glfwGetProcAddress(#name); \
        if (!name) return 0;                   \
    } while (0);
    L3D_GL_FUNCS(L3D_GL_LOAD)
#undef L3D_GL_LOAD
    return 1;
}
