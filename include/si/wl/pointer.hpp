#ifndef SI_WL_POINTER_HPP_INCLUDED
#define SI_WL_POINTER_HPP_INCLUDED

#include <wayland-client-protocol.h>
#include <memory>

namespace si::wl {
    struct pointer_deleter {
        void operator()(wl_pointer*) const;
    };
    class pointer {
        std::unique_ptr<wl_pointer, pointer_deleter> hnd;
        static void enter(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
        static void leave(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, wl_surface* surface);
        static void motion(void* data, wl_pointer* pointer_ptr, std::uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
        static void button(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, std::uint32_t time, std::uint32_t button, std::uint32_t state);
        static void axis(void* data, wl_pointer* pointer_ptr, std::uint32_t time, std::uint32_t axis, wl_fixed_t value);
        static void frame(void* data, wl_pointer* pointer_ptr);
        static void axis_source(void* data, wl_pointer* pointer_ptr, std::uint32_t axis_source);
        static void axis_click(void* data, wl_pointer* pointer_ptr, std::uint32_t time, std::uint32_t axis);
        static void axis_discrete(void* data, wl_pointer* pointer_ptr, std::uint32_t axis, std::int32_t discrete);
        const wl_pointer_listener listeners = { enter, leave, motion, button, axis, frame, axis_source, axis_click, axis_discrete };
    public:
        explicit pointer(wl_pointer*);
    };
}

#endif
