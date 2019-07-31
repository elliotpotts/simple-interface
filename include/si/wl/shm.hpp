#ifndef SI_WL_SHM_HPP_INCLUDED
#define SI_WL_SHM_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <si/wl/shm_pool.hpp>

namespace wl {
    struct shm_deleter {
        void operator()(wl_shm*) const;
    };
    class shm {
        std::unique_ptr<wl_shm, shm_deleter> const hnd;
        static void format(void* data, wl_shm* shm, std::uint32_t format);
        wl_shm_listener listener { format };
    public:
        explicit shm(wl_shm*);
        explicit operator wl_shm*() const;
        shm_pool make_pool(std::int32_t fd, std::int32_t size);
    };
}

#endif