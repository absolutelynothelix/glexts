#include <EGL/egl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Display *dpy;
XVisualInfo *vis;
PFNGLGETSTRINGIPROC glGetStringi;

int glx_exts_amt;
char **glx_exts;
int egl_exts_amt;
char **egl_exts;

void fetch_glx_exts() {
    GLXContext ctx = glXCreateContext(dpy, vis, NULL, True);
    glXMakeCurrent(dpy, None, ctx);

    glGetIntegerv(GL_NUM_EXTENSIONS, &glx_exts_amt);
    glx_exts = malloc(glx_exts_amt * sizeof(char *));
    for (int i = 0; i < glx_exts_amt; i++) {
        glx_exts[i] = (char *)glGetStringi(GL_EXTENSIONS, i);
    }

    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, ctx);
}

void fetch_egl_exts() {
    EGLDisplay display = eglGetDisplay(dpy);
    eglInitialize(display, NULL, NULL);

    EGLConfig config;
    int nconfigs = 1;
    eglChooseConfig(display, (EGLint[]){
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        // XXX(absolutelynothelix): I'm not sure about the alpha one.
        EGL_RED_SIZE, __builtin_popcountl(vis->red_mask),
        EGL_GREEN_SIZE, __builtin_popcountl(vis->green_mask),
        EGL_BLUE_SIZE, __builtin_popcountl(vis->blue_mask),
        EGL_CONFIG_CAVEAT, EGL_NONE,
        EGL_NONE
    }, &config, nconfigs, &nconfigs);

    eglBindAPI(EGL_OPENGL_API);

    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);

    glGetIntegerv(GL_NUM_EXTENSIONS, &egl_exts_amt);
    egl_exts = malloc(egl_exts_amt * sizeof(char *));
    for (int i = 0; i < egl_exts_amt; i++) {
        egl_exts[i] = (char *)glGetStringi(GL_EXTENSIONS, i);
    }

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, ctx);
    eglTerminate(display);
}

int main() {
    dpy = XOpenDisplay(NULL);

    XVisualInfo vinfo_template = {
        .visual = DefaultVisual(dpy, DefaultScreen(dpy))
    };

    int nitems_return;
    vis = XGetVisualInfo(dpy, VisualNoMask, &vinfo_template, &nitems_return);

    glGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddress("glGetStringi");

    fetch_glx_exts();
    printf("Fetched %d GLX extensions.\n", glx_exts_amt);

    fetch_egl_exts();
    printf("Fetched %d EGL extensions.\n", egl_exts_amt);

    XFree(vis);
    XCloseDisplay(dpy);

    int exts_amt = glx_exts_amt > egl_exts_amt ? glx_exts_amt : egl_exts_amt;

    int common_cnt = 0;
    char **common_exts = malloc(exts_amt * sizeof(char *));
    int glx_specific_cnt = 0;
    char **glx_specific_exts = malloc(exts_amt * sizeof(char *));
    int egl_specific_cnt = 0;
    char **egl_specific_exts = malloc(exts_amt * sizeof(char *));

    // XXX(absolutelynothelix): this is extremely inefficient, but I don't care.

    // First pass: find common extensions (that are both in glx_exts and
    // egl_exts) and GLX-specific extensions (that are in glx_exts but not in
    // egl_exts).
    for (int i = 0; i < glx_exts_amt; i++) {
        for (int j = 0; j < egl_exts_amt; j++) {
            if (strcmp(glx_exts[i], egl_exts[j]) == 0) {
                common_exts[common_cnt++] = glx_exts[i];
                break;
            } else if (j == egl_exts_amt - 1) {
                glx_specific_exts[glx_specific_cnt++] = glx_exts[i];
            }
        }
    }

    // Second pass: find EGL-specific extensions (that are in egl_exts but not
    // in glx_exts).
    for (int i = 0; i < egl_exts_amt; i++) {
        for (int j = 0; j < glx_exts_amt; j++) {
            if (strcmp(egl_exts[i], glx_exts[j]) == 0) {
                break;
            } else if (j == glx_exts_amt - 1) {
                egl_specific_exts[egl_specific_cnt++] = egl_exts[i];
            }
        }
    }

    free(glx_exts);
    free(egl_exts);

    printf("Common extensions (%d):\n", common_cnt);
    for (int i = 0; i < common_cnt; i++) {
        printf("\t%s\n", common_exts[i]);
    }

    free(common_exts);

    printf("GLX-specific extensions (%d):\n", glx_specific_cnt);
    for (int i = 0; i < glx_specific_cnt; i++) {
        printf("\t%s\n", glx_specific_exts[i]);
    }

    free(glx_specific_exts);

    printf("EGL-specific extensions (%d):\n", egl_specific_cnt);
    for (int i = 0; i < egl_specific_cnt; i++) {
        printf("\t%s\n", egl_specific_exts[i]);
    }

    free(egl_specific_exts);

    return 0;
}
