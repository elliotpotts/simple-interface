#include <si/wl/surface.hpp>

void wl::surface_deleter::operator()(wl_surface* sfc) const {
    wl_surface_destroy(sfc);
};
wl::surface::surface(wl_surface* surface) : hnd(surface) {
    if (!hnd) {
        throw std::runtime_error("Can't create surface from nullptr!");
    }
}
void wl::surface::dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data) {
    auto& signal = *reinterpret_cast<boost::signals2::signal<void(std::chrono::milliseconds)>*>(data);
    signal(std::chrono::milliseconds{callback_data});
}
void wl::surface::frame(boost::signals2::signal<void(std::chrono::milliseconds)>& signal) {
    auto frame_callback = wl_surface_frame(hnd.get());
    wl_callback_add_listener(frame_callback, &frame_listener, &signal);
}
wl::surface::operator wl_surface*() const {
    return hnd.get();
}
void wl::surface::attach(wl::buffer& buf, std::int32_t x, std::int32_t y) {
    wl_surface_attach(hnd.get(), static_cast<wl_buffer*>(buf), x, y);
}
void wl::surface::commit() {
    wl_surface_commit(hnd.get());
}
