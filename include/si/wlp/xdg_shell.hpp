#ifndef SI_WLP_XDG_SHELL_HPP_INCLUDED
#define SI_WLP_XDG_SHELL_HPP_INCLUDED

#include <wayland-client.h>
#include <wayland-client-xdg-shell.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <boost/signals2/signal.hpp>

namespace si::wlp {
    struct xdg_wm_base;
    struct xdg_positioner;
    struct xdg_surface;
    struct xdg_toplevel;
    struct xdg_popup;

    struct xdg_wm_base_deleter {
        void operator()(::xdg_wm_base*) const;
    };
    class xdg_wm_base {
        std::unique_ptr<::xdg_wm_base, xdg_wm_base_deleter> const hnd;
        static void handle_ping(void* data, ::xdg_wm_base*, std::uint32_t serial);
        ::xdg_wm_base_listener event_listener = { handle_ping };
    public:
        explicit xdg_wm_base(::xdg_wm_base*);
        boost::signals2::signal<void(std::uint32_t)> on_ping;
        xdg_positioner create_positioner();
        xdg_surface get_xdg_surface(wl_surface* surface);
        void pong(std::uint32_t serial);
    };
    struct xdg_positioner_deleter {
        void operator()(::xdg_positioner*) const;
    };
    class xdg_positioner {
        std::unique_ptr<::xdg_positioner, xdg_positioner_deleter> const hnd;
    public:
        explicit xdg_positioner(::xdg_positioner*);
        explicit operator ::xdg_positioner*() const;
        void set_size(std::int32_t width, std::int32_t height);
        void set_anchor_rect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
        void set_anchor(std::uint32_t anchor);
        void set_gravity(std::uint32_t gravity);
        void set_constraint_adjustment(std::uint32_t constraint_adjustment);
        void set_offset(std::int32_t x, std::int32_t y);
    };
    struct xdg_surface_deleter {
        void operator()(::xdg_surface*) const;
    };
    class xdg_surface {
        std::unique_ptr<::xdg_surface, xdg_surface_deleter> const hnd;
        static void handle_configure(void* data, ::xdg_surface*, std::uint32_t serial);
        ::xdg_surface_listener event_listener = { handle_configure };
    public:
        explicit xdg_surface(::xdg_surface*);
        explicit operator ::xdg_surface*() const;
        boost::signals2::signal<void(std::uint32_t)> on_configure;
        xdg_toplevel get_toplevel();
        xdg_popup get_popup(xdg_surface& parent, xdg_positioner& positioner);
        void set_window_geometry(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
        void ack_configure(std::uint32_t serial);
    };
    struct xdg_toplevel_deleter {
        void operator()(::xdg_toplevel*) const;
    };
    class xdg_toplevel {
        std::unique_ptr<::xdg_toplevel, xdg_toplevel_deleter> const hnd;
        static void handle_configure(void* data, ::xdg_toplevel*, std::int32_t width, std::int32_t height, wl_array* states);
        static void handle_close(void* data, ::xdg_toplevel*);
        ::xdg_toplevel_listener event_listener = { handle_configure, handle_close };
    public:
        explicit xdg_toplevel(::xdg_toplevel*);
        explicit operator ::xdg_toplevel*() const;
        boost::signals2::signal<void(std::int32_t, std::int32_t, ::wl_array*)> on_configure;
        boost::signals2::signal<void()> on_close;
        void set_parent(xdg_toplevel& parent);
        void set_title(std::string title);
        void set_app_id(std::string app_id);
        void show_window_menu(wl_seat* seat, std::uint32_t serial, std::int32_t x, std::int32_t y);
        void move(wl_seat* seat, std::uint32_t serial);
        void resize(wl_seat* seat, std::uint32_t serial, std::uint32_t edges);
        void set_max_size(std::int32_t width, std::int32_t height);
        void set_min_size(std::int32_t width, std::int32_t height);
        void set_maximized();
        void unset_maximized();
        void set_fullscreen(wl_output* output);
        void unset_fullscreen();
        void set_minimized();
    };
    struct xdg_popup_deleter {
        void operator()(::xdg_popup*) const;
    };
    class xdg_popup {
        std::unique_ptr<::xdg_popup, xdg_popup_deleter> const hnd;
        static void handle_configure(void* data, ::xdg_popup*, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
        static void handle_popup_done(void* data, ::xdg_popup*);
        ::xdg_popup_listener event_listener = { handle_configure, handle_popup_done };
    public:
        explicit xdg_popup(::xdg_popup*);
        boost::signals2::signal<void(std::int32_t, std::int32_t, std::int32_t, std::int32_t)> on_configure;
        boost::signals2::signal<void()> on_popup_done;
        void grab(wl_seat* seat, std::uint32_t serial);
    };
}

#endif


