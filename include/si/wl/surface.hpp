#ifndef SI_WL_SURFACE_HPP_INCLUDED
#define SI_WL_SURFACE_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <functional>
#include <chrono>
#include <si/wl/buffer.hpp>

namespace wl {
    struct surface_deleter {
        void operator()(wl_surface*) const;
    };
    class surface {
        std::unique_ptr<wl_surface, surface_deleter> const hnd;
        static void dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data);
        wl_callback* frame_callback = nullptr;
        wl_callback_listener frame_listener = { dispatch_frame };
        std::function<void(std::chrono::milliseconds)> frame_handler;
    public:
        explicit surface(wl_surface*);
        explicit operator wl_surface*() const;
        void attach(buffer& buf, std::int32_t x, std::int32_t y);
        void commit();
        template<typename F>
        void onframe(F&& f) {
            frame_handler = std::forward<F>(f);
            frame_callback = wl_surface_frame(hnd.get());
            wl_callback_add_listener(frame_callback, &frame_listener, this);
        }
    };
};

#endif
