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
        explicit compositor(wl_compositor*);
        ~compositor() = default;
        surface make_surface();
    };
}

#endif
