#ifndef SI_WL_SHELL_SURFACE_HPP_INCLUDED
#define SI_WL_SHELL_SURFACE_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>

namespace wl {
    struct shell_surface_deleter {
        void operator()(wl_shell_surface*) const;
    };
    class shell_surface {
        std::unique_ptr<wl_shell_surface, shell_surface_deleter> const hnd;      
        static void handle_ping(void*, wl_shell_surface*, std::uint32_t);          
        static void handle_configure(void*, wl_shell_surface*, std::uint32_t, std::int32_t, std::int32_t);
        static void handle_popup_done(void*, wl_shell_surface*);
        const wl_shell_surface_listener listener = {
            handle_ping,
            handle_configure,
            handle_popup_done
        };
    public:
        explicit shell_surface(wl_shell_surface*);
        ~shell_surface() = default;
        void set_toplevel();
    };
}

#endif
