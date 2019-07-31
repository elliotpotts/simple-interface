#include <si/wl/egl_window.hpp>
#include <fmt/format.h>
#include <si/egl.hpp>

#include <GL/gl.h>

void wl::egl_window_deleter::operator()(wl_egl_window* win) const {
    wl_egl_window_destroy(win);
}
wl::egl_window::egl_window(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context, surface& from, int w, int h):
    hnd {wl_egl_window_create(static_cast<wl_surface*>(from), w, h)} {
    if (hnd.get() == EGL_NO_SURFACE) {
        hnd.release();
        egl_throw();
    }
            
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, hnd.get(), nullptr);
    if (egl_surface == EGL_NO_SURFACE) {
        egl_throw();
    }

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fmt::print("egl made current.\n");
    } else {
        egl_throw();
    }

    glClearColor(1.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fmt::print("egl buffer swapped\n");
    } else {
        egl_throw();
    }
}
