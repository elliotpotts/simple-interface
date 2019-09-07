#include <si/X/xcb_display.hpp>

void si::X::xcb_display_deleter::operator()(xcb_connection_t* dpy) const {
    xcb_disconnect(dpy);
}
si::X::xcb_display::xcb_display() : hnd(xcb_connect(nullptr, nullptr)) {
    //TODO: how do detect erorrs?
}
