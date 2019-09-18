#ifndef SI_WL_DISPLAY_HPP_INCLUDED
#define SI_WL_DISPLAY_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <si/wl/registry.hpp>
/* Note: wayland-egl.h must be included before EGL/egl.h to ensure that EGLNativeDisplayType is wl_display and not XDisplay */
#include <wayland-egl.h>
#include <EGL/egl.h>

namespace wl {
    struct display_deleter {
        void operator()(wl_display*) const;
    };
    class display {
        std::unique_ptr<wl_display, display_deleter> const hnd;
    public:
        display();
        explicit operator wl_display*();
        registry make_registry();
        int dispatch();
        void roundtrip();
        EGLDisplay egl();
    };
};
#endif
