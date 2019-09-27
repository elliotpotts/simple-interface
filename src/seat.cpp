#include <si/wl/seat.hpp>
#include <si/wl/pointer.hpp>
#include <spdlog/spdlog.h>

void si::wl::seat_deleter::operator()(wl_seat* seat_ptr) const {
    wl_seat_destroy(seat_ptr);
}
void si::wl::seat::capabilities(void* data, wl_seat* seat_ptr, std::uint32_t capabilities) {
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        spdlog::info("Incoming seat has pointer");
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        spdlog::info("Incoming seat has keyboard");
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
        spdlog::info("Incoming seat has touch");
    }
}
void si::wl::seat::name(void* data, wl_seat* seat, const char* name) {
    spdlog::info("Seat \"{}\" arrived.", name);
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
