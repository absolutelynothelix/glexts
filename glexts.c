#include <EGL/egl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Display *dpy;

PFNGLGETSTRINGIPROC glGetStringi;

int glx_num_extensions;
char **glx_extensions;

int egl_num_extensions;
char **egl_extensions;

void fetch_glx_extensions() {
    XVisualInfo vinfo_template = {
        .visual = DefaultVisual(dpy, DefaultScreen(dpy))
    };

    int nitems_return;
    XVisualInfo *vis = XGetVisualInfo(dpy, VisualNoMask, &vinfo_template, &nitems_return);

    GLXContext ctx = glXCreateContext(dpy, vis, NULL, True);
    glXMakeCurrent(dpy, None, ctx);

    glGetIntegerv(GL_NUM_EXTENSIONS, &glx_num_extensions);
    glx_extensions = malloc(glx_num_extensions * sizeof(char *));
    for (int i = 0; i < glx_num_extensions; i++) {
        glx_extensions[i] = (char *)glGetStringi(GL_EXTENSIONS, i);
    }

    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, ctx);
}

void fetch_egl_extensions() {
    EGLDisplay display = eglGetDisplay(dpy);
    eglInitialize(display, NULL, NULL);

    int egl_num_configs = 0;
    eglGetConfigs(display, NULL, 0, &egl_num_configs);

    EGLConfig config;
    eglChooseConfig(display, (EGLint[]){
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_CONFIG_CAVEAT, EGL_NONE,
        EGL_NONE
    }, &config, 1, &egl_num_configs);

    eglBindAPI(EGL_OPENGL_API);

    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);

    glGetIntegerv(GL_NUM_EXTENSIONS, &egl_num_extensions);
    egl_extensions = malloc(egl_num_extensions * sizeof(char *));
    for (int i = 0; i < egl_num_extensions; i++) {
        egl_extensions[i] = (char *)glGetStringi(GL_EXTENSIONS, i);
    }
}

int main() {
    dpy = XOpenDisplay(NULL);

    glGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddress("glGetStringi");

    fetch_glx_extensions();
    fetch_egl_extensions();

    int size = glx_num_extensions > egl_num_extensions ? glx_num_extensions : egl_num_extensions;
    char **common = malloc(size * sizeof(char *));
    int common_cnt = 0;
    char **glx_specific = malloc(size * sizeof(char *));
    int glx_specific_cnt = 0;
    char **egl_specific = malloc(size * sizeof(char *));
    int egl_specific_cnt = 0;

    for (int i = 0; i < glx_num_extensions; i++) {
        char* glx_ext = glx_extensions[i];
        for (int j = 0; j < egl_num_extensions; j++) {
            char* egl_ext = egl_extensions[j];
            if (strcmp(glx_ext, egl_ext) == 0) {
                common[common_cnt++] = glx_ext;
                break;
            } else if (j == egl_num_extensions - 1) {
                glx_specific[glx_specific_cnt++] = glx_ext;
            }
        }
    }

    for (int i = 0; i < egl_num_extensions; i++) {
        char* egl_ext = egl_extensions[i];
        for (int j = 0; j < glx_num_extensions; j++) {
            char* glx_ext = glx_extensions[j];
            if (strcmp(egl_ext, glx_ext) == 0) {
                break;
            } else if (j == glx_num_extensions - 1) {
                egl_specific[egl_specific_cnt++] = egl_ext;
            }
        }
    }

    free(glx_extensions);
    free(egl_extensions);

    printf("Common extensions (%d):\n", common_cnt);
    for (int i = 0; i < common_cnt; i++) {
        printf("\t%s\n", common[i]);
    }

    free(common);

    printf("GLX-specific extensions (%d):\n", glx_specific_cnt);
    for (int i = 0; i < glx_specific_cnt; i++) {
        printf("\t%s\n", glx_specific[i]);
    }

    free(glx_specific);

    printf("EGL-specific extensions (%d):\n", egl_specific_cnt);
    for (int i = 0; i < egl_specific_cnt; i++) {
        printf("\t%s\n", egl_specific[i]);
    }

    free(egl_specific);

    XCloseDisplay(dpy);
}