#ifndef SI_UI_HPP_INCLUDED
#define SI_UI_HPP_INCLUDED

#include <variant>
#include <string>

namespace si {
    struct window {
        int width;
        int height;
    };
    // text
    // rect
    // image
    // layouts
    // etc.
    struct rect {
        std::uint32_t colour;
    };
    void run(const window&);
}

#endif
