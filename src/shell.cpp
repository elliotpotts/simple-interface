#include <si/wl/shell.hpp>

void wl::shell_deleter::operator()(wl_shell* shell) const {
    wl_shell_destroy(shell);
}
wl::shell::shell(wl_shell* shell) : hnd(shell) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell from nullptr!");
    }
}
wl::shell_surface wl::shell::make_surface(const surface& surface) {
    return wl::shell_surface{ wl_shell_get_shell_surface(hnd.get(), static_cast<wl_surface*>(surface)) };
}
