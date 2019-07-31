#include <si/wl/shm.hpp>
#include <fmt/format.h>

void wl::shm_deleter::operator()(wl_shm* shm) const {
    wl_shm_destroy(shm);
}
wl::shm::shm(wl_shm* shm) : hnd(shm) {
    if (!hnd) {
        throw std::runtime_error("Can't create shm from nullptr!");
    }
    wl_shm_add_listener(shm, &listener, nullptr);
}
void wl::shm::format(void* data, wl_shm* shm, std::uint32_t format) {
    fmt::print("Format {}\n", format);
}
wl::shm::operator wl_shm*() const {
    return hnd.get();
}
wl::shm_pool wl::shm::make_pool(std::int32_t fd, std::int32_t size) {
    return wl::shm_pool { wl_shm_create_pool(hnd.get(), fd, size) };
}
