#ifndef SI_WL_EGL_WINDOW_HPP_INLUDED
#define SI_WL_EGL_WINDOW_HPP_INLUDED

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <memory>
#include <si/wl/surface.hpp>

namespace wl {
    struct egl_window_deleter {
        void operator()(wl_egl_window* win) const;
    };
    class egl_window {
        std::unique_ptr<wl_egl_window, egl_window_deleter> hnd;
        EGLSurface egl_surface;
    public:
        egl_window(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context, surface& from, int w, int h);
        EGLSurface surface() const;
    };
}

#endif
