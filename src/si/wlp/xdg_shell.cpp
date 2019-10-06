#include <si/wlp/xdg_shell.hpp>

void si::wlp::xdg_wm_base_deleter::operator()(::xdg_wm_base* ptr) const {
    ::xdg_wm_base_destroy(ptr);
}
void si::wlp::xdg_wm_base::handle_ping(void* data, ::xdg_wm_base*, std::uint32_t serial) {
    xdg_wm_base& wrapper = *reinterpret_cast<xdg_wm_base*>(data);
    wrapper.on_ping(serial);
}
si::wlp::xdg_wm_base::xdg_wm_base(::xdg_wm_base* ptr): hnd(ptr) {
    ::xdg_wm_base_add_listener(hnd.get(), &event_listener, this);
}
si::wlp::xdg_positioner si::wlp::xdg_wm_base::create_positioner() {
    return xdg_positioner(::xdg_wm_base_create_positioner(hnd.get()));
}
si::wlp::xdg_surface si::wlp::xdg_wm_base::get_xdg_surface(wl_surface* surface) {
    return xdg_surface(::xdg_wm_base_get_xdg_surface(hnd.get(), surface));
}
void si::wlp::xdg_wm_base::pong(std::uint32_t serial) {
    return void(::xdg_wm_base_pong(hnd.get(), serial));
}
void si::wlp::xdg_positioner_deleter::operator()(::xdg_positioner* ptr) const {
    ::xdg_positioner_destroy(ptr);
}
si::wlp::xdg_positioner::xdg_positioner(::xdg_positioner* ptr): hnd(ptr) {
}
si::wlp::xdg_positioner::operator ::xdg_positioner*() const {
    return hnd.get();
}
void si::wlp::xdg_positioner::set_size(std::int32_t width, std::int32_t height) {
    return void(::xdg_positioner_set_size(hnd.get(), width, height));
}
void si::wlp::xdg_positioner::set_anchor_rect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) {
    return void(::xdg_positioner_set_anchor_rect(hnd.get(), x, y, width, height));
}
void si::wlp::xdg_positioner::set_anchor(std::uint32_t anchor) {
    return void(::xdg_positioner_set_anchor(hnd.get(), anchor));
}
void si::wlp::xdg_positioner::set_gravity(std::uint32_t gravity) {
    return void(::xdg_positioner_set_gravity(hnd.get(), gravity));
}
void si::wlp::xdg_positioner::set_constraint_adjustment(std::uint32_t constraint_adjustment) {
    return void(::xdg_positioner_set_constraint_adjustment(hnd.get(), constraint_adjustment));
}
void si::wlp::xdg_positioner::set_offset(std::int32_t x, std::int32_t y) {
    return void(::xdg_positioner_set_offset(hnd.get(), x, y));
}
void si::wlp::xdg_surface_deleter::operator()(::xdg_surface* ptr) const {
    ::xdg_surface_destroy(ptr);
}
void si::wlp::xdg_surface::handle_configure(void* data, ::xdg_surface*, std::uint32_t serial) {
    xdg_surface& wrapper = *reinterpret_cast<xdg_surface*>(data);
    wrapper.on_configure(serial);
}
si::wlp::xdg_surface::xdg_surface(::xdg_surface* ptr): hnd(ptr) {
    ::xdg_surface_add_listener(hnd.get(), &event_listener, this);
}
si::wlp::xdg_surface::operator ::xdg_surface*() const {
    return hnd.get();
}
si::wlp::xdg_toplevel si::wlp::xdg_surface::get_toplevel() {
    return xdg_toplevel(::xdg_surface_get_toplevel(hnd.get()));
}
si::wlp::xdg_popup si::wlp::xdg_surface::get_popup(xdg_surface& parent, xdg_positioner& positioner) {
    return xdg_popup(::xdg_surface_get_popup(hnd.get(), static_cast<::xdg_surface*>(parent), static_cast<::xdg_positioner*>(positioner)));
}
void si::wlp::xdg_surface::set_window_geometry(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) {
    return void(::xdg_surface_set_window_geometry(hnd.get(), x, y, width, height));
}
void si::wlp::xdg_surface::ack_configure(std::uint32_t serial) {
    return void(::xdg_surface_ack_configure(hnd.get(), serial));
}
void si::wlp::xdg_toplevel_deleter::operator()(::xdg_toplevel* ptr) const {
    ::xdg_toplevel_destroy(ptr);
}
void si::wlp::xdg_toplevel::handle_configure(void* data, ::xdg_toplevel*, std::int32_t width, std::int32_t height, wl_array* states) {
    xdg_toplevel& wrapper = *reinterpret_cast<xdg_toplevel*>(data);
    wrapper.on_configure(width, height, states);
}
void si::wlp::xdg_toplevel::handle_close(void* data, ::xdg_toplevel*) {
    xdg_toplevel& wrapper = *reinterpret_cast<xdg_toplevel*>(data);
    wrapper.on_close();
}
si::wlp::xdg_toplevel::xdg_toplevel(::xdg_toplevel* ptr): hnd(ptr) {
    ::xdg_toplevel_add_listener(hnd.get(), &event_listener, this);
}
si::wlp::xdg_toplevel::operator ::xdg_toplevel*() const {
    return hnd.get();
}
void si::wlp::xdg_toplevel::set_parent(xdg_toplevel& parent) {
    return void(::xdg_toplevel_set_parent(hnd.get(), static_cast<::xdg_toplevel*>(parent)));
}
void si::wlp::xdg_toplevel::set_title(std::string title) {
    return void(::xdg_toplevel_set_title(hnd.get(), title.c_str()));
}
void si::wlp::xdg_toplevel::set_app_id(std::string app_id) {
    return void(::xdg_toplevel_set_app_id(hnd.get(), app_id.c_str()));
}
void si::wlp::xdg_toplevel::show_window_menu(wl_seat* seat, std::uint32_t serial, std::int32_t x, std::int32_t y) {
    return void(::xdg_toplevel_show_window_menu(hnd.get(), seat, serial, x, y));
}
void si::wlp::xdg_toplevel::move(wl_seat* seat, std::uint32_t serial) {
    return void(::xdg_toplevel_move(hnd.get(), seat, serial));
}
void si::wlp::xdg_toplevel::resize(wl_seat* seat, std::uint32_t serial, std::uint32_t edges) {
    return void(::xdg_toplevel_resize(hnd.get(), seat, serial, edges));
}
void si::wlp::xdg_toplevel::set_max_size(std::int32_t width, std::int32_t height) {
    return void(::xdg_toplevel_set_max_size(hnd.get(), width, height));
}
void si::wlp::xdg_toplevel::set_min_size(std::int32_t width, std::int32_t height) {
    return void(::xdg_toplevel_set_min_size(hnd.get(), width, height));
}
void si::wlp::xdg_toplevel::set_maximized() {
    return void(::xdg_toplevel_set_maximized(hnd.get()));
}
void si::wlp::xdg_toplevel::unset_maximized() {
    return void(::xdg_toplevel_unset_maximized(hnd.get()));
}
void si::wlp::xdg_toplevel::set_fullscreen(wl_output* output) {
    return void(::xdg_toplevel_set_fullscreen(hnd.get(), output));
}
void si::wlp::xdg_toplevel::unset_fullscreen() {
    return void(::xdg_toplevel_unset_fullscreen(hnd.get()));
}
void si::wlp::xdg_toplevel::set_minimized() {
    return void(::xdg_toplevel_set_minimized(hnd.get()));
}
void si::wlp::xdg_popup_deleter::operator()(::xdg_popup* ptr) const {
    ::xdg_popup_destroy(ptr);
}
void si::wlp::xdg_popup::handle_configure(void* data, ::xdg_popup*, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) {
    xdg_popup& wrapper = *reinterpret_cast<xdg_popup*>(data);
    wrapper.on_configure(x, y, width, height);
}
void si::wlp::xdg_popup::handle_popup_done(void* data, ::xdg_popup*) {
    xdg_popup& wrapper = *reinterpret_cast<xdg_popup*>(data);
    wrapper.on_popup_done();
}
si::wlp::xdg_popup::xdg_popup(::xdg_popup* ptr): hnd(ptr) {
    ::xdg_popup_add_listener(hnd.get(), &event_listener, this);
}
void si::wlp::xdg_popup::grab(wl_seat* seat, std::uint32_t serial) {
    return void(::xdg_popup_grab(hnd.get(), seat, serial));
}
