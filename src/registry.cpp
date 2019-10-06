#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/wl/shm.hpp>
#include <si/wl/seat.hpp>
#include <si/wlp/xdg_shell.hpp>
#include <spdlog/spdlog.h>
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
void wl::registry::handler(void* data, wl_registry* reg_ptr, std::uint32_t id, const char* iface, std::uint32_t version) {
    auto& reg = *reinterpret_cast<wl::registry*>(data);
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
    } else if (iface == "wl_seat"s) {
        reg.objects.emplace_front (
            std::in_place_type<si::wl::seat>,
            static_cast<wl_seat*> (wl_registry_bind(reg_ptr, id, &wl_seat_interface, version))
        );
    } else if (iface == "xdg_wm_base"s) {
        reg.objects.emplace_front (
            std::in_place_type<si::wlp::xdg_wm_base>,
            static_cast<xdg_wm_base*> (wl_registry_bind(reg_ptr, id, &xdg_wm_base_interface, version))
        );
    } else {
        spdlog::info("{:>3}: {}: no handler", id, iface);
        return;
    }
    spdlog::info("{:>3}: {}: bound successfully", id, iface);
}
void wl::registry::remover(void* data, wl_registry* registry, std::uint32_t id) {
    spdlog::debug("Got a registry losing event for id {}", id);
}
wl::registry::operator wl_registry*() const {
    return hnd.get();
}
