#ifndef SI_WL_SURFACE_HPP_INCLUDED
#define SI_WL_SURFACE_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <functional>
#include <chrono>
#include <si/wl/buffer.hpp>
#include <boost/signals2/signal.hpp>

namespace wl {
    struct surface_deleter {
        void operator()(wl_surface*) const;
    };
    class surface {
        std::unique_ptr<wl_surface, surface_deleter> const hnd;
        static void dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data);
        wl_callback_listener frame_listener = { dispatch_frame };
    public:
        explicit surface(wl_surface*);
        explicit operator wl_surface*() const;
        void attach(buffer& buf, std::int32_t x, std::int32_t y);
        void commit();
        void frame(boost::signals2::signal<void(std::chrono::milliseconds)>& signal);
    };
};

#endif
