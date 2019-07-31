#ifndef SI_WL_SHELL_HPP_INCLUDED
#define SI_WL_SHELL_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <si/wl/surface.hpp>
#include <si/wl/shell_surface.hpp>

namespace wl {
    struct shell_deleter {
        void operator()(wl_shell*) const;
    };
    class shell {
        std::unique_ptr<wl_shell, shell_deleter> const hnd;
    public:
        explicit shell(wl_shell*);
        explicit operator const wl_shell*() const;
        shell_surface make_surface(const surface&);
    };
}

#endif
