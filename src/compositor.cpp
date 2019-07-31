#include <si/wl/compositor.hpp>

void wl::compositor_deleter::operator()(wl_compositor* comp) const {
    wl_compositor_destroy(comp);
}
wl::compositor::compositor(wl_compositor* compositor) : hnd(compositor) {
    if (!hnd) {
        throw std::runtime_error("Can't create compositor from nullptr!");
    }
}
wl::surface wl::compositor::make_surface() {
    return wl::surface{ wl_compositor_create_surface(hnd.get()) };
}
