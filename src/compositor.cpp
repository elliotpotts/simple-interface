#include <si/wl/compositor.hpp>

void wl::compositor_deleter::operator()(wl_compositor* comp) const {
    wl_compositor_destroy(comp);
}
wl::compositor::compositor(::wl_registry* reg_ptr, std::uint32_t id, std::uint32_t version):
    hnd(static_cast<::wl_compositor*>(wl_registry_bind(reg_ptr, id, &wl_compositor_interface, version)))
    {
    if (!hnd) {
        throw std::runtime_error("Can't create compositor from nullptr!");
    }
}
wl::surface wl::compositor::make_surface() {
    return wl::surface{ wl_compositor_create_surface(hnd.get()) };
}
