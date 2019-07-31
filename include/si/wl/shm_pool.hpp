#ifndef SI_WL_SHM_POOL_HPP_INCLUDED
#define SI_WL_SHM_POOL_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <cstdint>
#include <si/wl/buffer.hpp>

namespace wl {
    struct shm_pool_deleter {
        void operator()(wl_shm_pool*) const;
    };
    class shm_pool {
        std::unique_ptr<wl_shm_pool, shm_pool_deleter> const hnd;
    public:
        explicit shm_pool(wl_shm_pool*);
        explicit operator wl_shm_pool*() const;
        buffer make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format);
    };
}

#endif
