#include <si/wl/seat.hpp>
#include <si/wl/pointer.hpp>
#include <fmt/format.h>

void si::wl::seat_deleter::operator()(wl_seat* seat_ptr) const {
    wl_seat_destroy(seat_ptr);
}
void si::wl::seat::capabilities(void* data, wl_seat* seat_ptr, std::uint32_t capabilities) {
    auto& seat = *reinterpret_cast<si::wl::seat*>(data);
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        fmt::print("Incoming seat has pointer\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        fmt::print("Incoming seat has keyboard\n");
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
        fmt::print("Incoming seat has touch\n");
    }
}
void si::wl::seat::name(void* data, wl_seat* seat, const char* name) {
    fmt::print("Seat \"{}\" arrived.\n", name);
}
si::wl::pointer si::wl::seat::pointer() const {
    return si::wl::pointer(wl_seat_get_pointer(hnd.get()));
}
si::wl::keyboard si::wl::seat::keyboard() const {
    return si::wl::keyboard(wl_seat_get_keyboard(hnd.get()));
}
si::wl::seat::seat(wl_seat* seat) : hnd(seat) {
    wl_seat_add_listener(hnd.get(), &seat_listener, this);
}
