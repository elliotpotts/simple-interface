#ifndef SI_WL_SEAT_HPP_INCLUDED
#define SI_WL_SEAT_HPP_INCLUDED

#include <wayland-client-protocol.h>
#include <memory>
#include <si/wl/pointer.hpp>
#include <si/wl/keyboard.hpp>

namespace si::wl {
    struct seat_deleter {
        void operator()(wl_seat*) const;
    };
    class seat {
        std::unique_ptr<wl_seat, seat_deleter> const hnd;
        static void capabilities(void* data, wl_seat* seat, std::uint32_t capabilities);
        static void name(void* data, wl_seat* seat, const char* name);
        const wl_seat_listener seat_listener { capabilities, name };
    public:
        explicit seat(wl_seat*);
        explicit operator const wl_seat*() const;
        si::wl::pointer pointer() const;
        si::wl::keyboard keyboard() const;
    };
}

#endif
