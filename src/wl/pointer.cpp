#include <si/wl/pointer.hpp>
#include <fmt/format.h>

void si::wl::pointer_deleter::operator()(wl_pointer* pointer) const {
    wl_pointer_destroy(pointer);
}
void si::wl::pointer::enter(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {
}
void si::wl::pointer::leave(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, wl_surface* surface) {
}
void si::wl::pointer::motion(void* data, wl_pointer* pointer_ptr, std::uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
}
void si::wl::pointer::button(void* data, wl_pointer* pointer_ptr, std::uint32_t serial, std::uint32_t time, std::uint32_t button, std::uint32_t state) {
}
void si::wl::pointer::axis(void* data, wl_pointer* pointer_ptr, std::uint32_t time, std::uint32_t axis, wl_fixed_t value) {
}
void si::wl::pointer::frame(void* data, wl_pointer* pointer_ptr) {
}
void si::wl::pointer::axis_source(void* data, wl_pointer* pointer_ptr, std::uint32_t axis_source) {
}
void si::wl::pointer::axis_click(void* data, wl_pointer* pointer_ptr, std::uint32_t time, std::uint32_t axis) {
}
void si::wl::pointer::axis_discrete(void* data, wl_pointer* pointer_ptr, std::uint32_t axis, std::int32_t discrete) {
}
si::wl::pointer::pointer(wl_pointer* pointer) : hnd(pointer) {
    wl_pointer_add_listener(pointer, &listeners, this);
}
