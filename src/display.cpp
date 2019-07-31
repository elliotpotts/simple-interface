#include <si/wl/display.hpp>
#include <si/egl.hpp>

void wl::display_deleter::operator()(wl_display* dpy) const {
    wl_display_disconnect(dpy);
}
wl::display::display() : hnd(wl_display_connect(nullptr)) {
    if (!hnd) {
        throw std::runtime_error("Can't connect to wayland");
    }
}
wl::registry wl::display::make_registry() {
    return wl::registry{wl_display_get_registry(hnd.get())};
}
int wl::display::dispatch() {
    int count = wl_display_dispatch(hnd.get());
    if (count == -1) {
        throw std::runtime_error("Dispatch failed!");
    } else {
        return count;
    }
}
void wl::display::roundtrip() {
    wl_display_roundtrip(hnd.get());
}
EGLDisplay wl::display::egl() {
    if (EGLDisplay dpy = eglGetDisplay(hnd.get()); dpy == EGL_NO_DISPLAY) {
        egl_throw();
    } else {
        return dpy;
    }
}
