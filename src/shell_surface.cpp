#include <si/wl/shell_surface.hpp>
#include <spdlog/spdlog.h>

void wl::shell_surface_deleter::operator()(wl_shell_surface* sfc) const {
    wl_shell_surface_destroy(sfc);
}
wl::shell_surface::shell_surface(wl_shell_surface* surface) : hnd(surface) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell_surface from nullptr!");
    }
    wl_shell_surface_add_listener(hnd.get(), &listener, nullptr);
}
void wl::shell_surface::handle_ping(void* data, wl_shell_surface* shell_surface, std::uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
    spdlog::debug("Pinged and ponged");
}
void wl::shell_surface::handle_configure(void*, wl_shell_surface*, std::uint32_t, std::int32_t, std::int32_t) {
}
void wl::shell_surface::handle_popup_done(void*, wl_shell_surface*) {
}
void wl::shell_surface::set_toplevel() {
    wl_shell_surface_set_toplevel(hnd.get());
}
