#include <si/wl/egl_window.hpp>
#include <fmt/format.h>
#include <si/egl.hpp>

void wl::egl_window_deleter::operator()(wl_egl_window* win) const {
    wl_egl_window_destroy(win);
}
wl::egl_window::egl_window(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context, wl::surface& from, int w, int h):
    hnd {wl_egl_window_create(static_cast<wl_surface*>(from), w, h)} {
    if (hnd.get() == EGL_NO_SURFACE) {
        hnd.release();
        egl_throw();
    }
            
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, hnd.get(), nullptr);
    if (egl_surface == EGL_NO_SURFACE) {
        egl_throw();
    }
}
EGLSurface wl::egl_window::surface() const {
    return egl_surface;
}
