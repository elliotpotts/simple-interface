#ifndef SI_X_GLX_DISPLAY_HPP_INCLUDED
#define SI_X_GLX_DISPLAY_HPP_INCLUDED

#include <memory>
#include <si/X/xcb_display.hpp>
#include <X11/Xlib.h>

namespace si::X {   
    struct glx_display_deleter {
        void operator()(Display*) const;
    };
    class glx_display {
        std::unique_ptr<Display, glx_display_deleter> const hnd;
        xcb_display xcb;
    public:
        glx_display();
    };
}

#endif
