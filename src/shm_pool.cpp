#include <si/wl/shm_pool.hpp>

void wl::shm_pool_deleter::operator()(wl_shm_pool* pool) const {
    wl_shm_pool_destroy(pool);
}
wl::shm_pool::shm_pool(wl_shm_pool* pool) : hnd(pool) {
    if (!hnd) {
        throw std::runtime_error("Can't create pool from nullptr!");
    }
}
wl::shm_pool::operator wl_shm_pool*() const {
    return hnd.get();
}
wl::buffer wl::shm_pool::make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format) {
    return wl::buffer{ wl_shm_pool_create_buffer(hnd.get(), offset, width, height, stride, format) };
}
