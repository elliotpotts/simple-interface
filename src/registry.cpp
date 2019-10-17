#include <si/wl/registry.hpp>
#include <spdlog/spdlog.h>

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
    auto [it, inserted] = reg.proto_ifaces.emplace(std::string{iface}, std::make_tuple(reg_ptr, id, version));
    if (!inserted) {
        spdlog::error("{:>3}: {}: duplicate!");
    } else {
        spdlog::info("{:>3}: {}: entered", id, iface);
    }
}
void wl::registry::remover(void* data, wl_registry* registry, std::uint32_t id) {
    spdlog::debug("Got a registry losing event for id {}", id);
}
wl::registry::operator wl_registry*() const {
    return hnd.get();
}
