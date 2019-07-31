#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/wl/shm.hpp>
#include <fmt/format.h>
#include <string>

using namespace std::string_literals;

void wl::registry_deleter::operator()(wl_registry* reg) const {
    wl_registry_destroy(reg);
}
wl::registry::registry(wl_registry* registry) : hnd(registry) {
    if (!hnd) {
        throw std::runtime_error("Can't create registry from nullptr!");
    }
    wl_registry_add_listener(hnd.get(), &listener, this);
}
void wl::registry::handler(void* data, wl_registry* registry, std::uint32_t id, const char* iface, std::uint32_t version) {
    auto& reg = *reinterpret_cast<wl::registry*>(data);
    auto reg_ptr = static_cast<wl_registry*>(reg);
    if (iface == "wl_compositor"s) {
        reg.objects.emplace_front (
            std::in_place_type<compositor>,
            static_cast<wl_compositor*> (wl_registry_bind(reg_ptr, id, &wl_compositor_interface, version))
        );
    } else if (iface == "wl_shell"s) {
        reg.objects.emplace_front (
            std::in_place_type<shell>,
            static_cast<wl_shell*> (wl_registry_bind(reg_ptr, id, &wl_shell_interface, version))
        );
    } else if (iface == "wl_shm"s) {
        reg.objects.emplace_front (
            std::in_place_type<shm>,
            static_cast<wl_shm*> (wl_registry_bind(reg_ptr, id, &wl_shm_interface, version))
        );
    }
    else {
        fmt::print("{:>3}: {}: no handler\n", id, iface);
        return;
    }
    fmt::print("{:>3}: {}: bound successfully\n", id, iface);
}
void wl::registry::remover(void* data, wl_registry* registry, std::uint32_t id) {
    fmt::print("Got a registry losing event for id {}\n", id);
}
wl::registry::operator wl_registry*() const {
    return hnd.get();
}
