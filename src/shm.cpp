#include <si/wl/shm.hpp>
#include <spdlog/spdlog.h>

void wl::shm_deleter::operator()(wl_shm* shm) const {
    wl_shm_destroy(shm);
}
wl::shm::shm(::wl_registry* reg_ptr, std::uint32_t id, std::uint32_t version):
    hnd(static_cast<wl_shm*>(wl_registry_bind(reg_ptr, id, &wl_shm_interface, version)))
    {
    if (!hnd) {
        throw std::runtime_error("Can't create shm from nullptr!");
    }
    wl_shm_add_listener(hnd.get(), &listener, this);
}
void wl::shm::dispatch_format(void* data, wl_shm* shm, std::uint32_t format) {
    spdlog::debug("Format {} is valid", format);
}
wl::shm::operator wl_shm*() const {
    return hnd.get();
}
wl::shm_pool wl::shm::make_pool(std::int32_t fd, std::int32_t size) {
    return wl::shm_pool { wl_shm_create_pool(hnd.get(), fd, size) };
}
