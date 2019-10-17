#ifndef SI_WL_COMPOSITOR_HPP_INCLUDED
#define SI_WL_COMPOSITOR_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <si/wl/surface.hpp>

namespace wl {
    struct compositor_deleter {
        void operator()(wl_compositor*) const;
    };
    class compositor {
        std::unique_ptr<wl_compositor, compositor_deleter> const hnd;
    public:
        explicit compositor(::wl_registry* reg_ptr, std::uint32_t id, std::uint32_t version);
        ~compositor() = default;
        static constexpr const char* wl_interface_name = "wl_compositor";
        surface make_surface();
    };
}

#endif
