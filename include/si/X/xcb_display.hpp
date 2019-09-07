#ifndef SI_X_XCB_DISPLAY_HPP_INCLUDED
#define SI_X_XCB_DISPLAY_HPP_INCLUDED

#include <memory>
#include <xcb/xcb.h>

namespace si::X {
    struct xcb_display_deleter {
        void operator()(xcb_connection_t*) const;
    };
    class xcb_display {
        std::unique_ptr<xcb_connection_t, xcb_display_deleter> const hnd;
    public:
        xcb_display();
    };
}

#endif
