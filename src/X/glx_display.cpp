#include <si/X/glx_display.hpp>
#include <X11/Xlib.h>

void si::X::glx_display_deleter::operator()(Display* dpy) const {
    //
}
si::X::glx_display::glx_display() : hnd(XOpenDisplay(nullptr)) {
}
