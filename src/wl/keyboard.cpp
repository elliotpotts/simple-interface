#include <si/wl/keyboard.hpp>
#include <fmt/format.h>
void si::wl::keyboard_deleter::operator()(wl_keyboard* keybd) const {
    wl_keyboard_destroy(keybd);
}
void si::wl::keyboard::keymap(void* data, wl_keyboard* keyboard, std::uint32_t format, std::int32_t fd, std::uint32_t size) {
}
void si::wl::keyboard::enter(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface, wl_array* keys) {
}
void si::wl::keyboard::leave(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface) {
}
void si::wl::keyboard::key(void* data, wl_keyboard* keyboard, std::uint32_t serial, std::uint32_t time, std::uint32_t key, std::uint32_t state) {
    fmt::print("Keypress - Goodbye!", key);
    std::terminate();
}
void si::wl::keyboard::modifiers(void* data, wl_keyboard* keyboard, std::uint32_t serial, std::uint32_t mods_depressed, std::uint32_t mods_latched, std::uint32_t mods_locked, std::uint32_t group) {
}
void si::wl::keyboard::repeat_info(void* data, wl_keyboard* keyboard, std::int32_t rate, std::int32_t delay) {
}
si::wl::keyboard::keyboard(wl_keyboard* keyboard): hnd(keyboard) {
    wl_keyboard_add_listener(keyboard, &listeners, this);
}
