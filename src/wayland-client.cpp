#include <fmt/format.h>
#include <algorithm>

#include <si/wl/display.hpp>
#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/wl/shm.hpp>

#include <si/egl.hpp>
#include <si/wl/egl_window.hpp>
#include <si/shm_buffer.hpp>

#include <GL/gl.h>

inline constexpr int win_width = 480;
inline constexpr int win_height = 360;

int main() {
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    //my_display.dispatch(); // TODO: Figure out when I need to dispatch/roundtrip
    my_display.roundtrip();

    auto& my_compositor = my_registry.get<wl::compositor>();
    auto& my_shell = my_registry.get<wl::shell>();

    wl::surface my_surface = my_compositor.make_surface();
    wl::shell_surface my_shell_surface = my_shell.make_surface(my_surface);
    my_shell_surface.set_toplevel();

    bool use_acceleration = false;
    if (use_acceleration) {
        EGLDisplay egl_display = my_display.egl();
        auto [egl_context, egl_config] = init_egl(egl_display);
        describe_egl_config(egl_display, egl_config);
        wl::egl_window egl_window(egl_display, egl_config, egl_context, my_surface, win_width, win_height);

        /* Paint on the freshly created rendering context */
        if (eglMakeCurrent(egl_display, egl_window.surface(), egl_window.surface(), egl_context)) {
            fmt::print("egl made current.\n");
        } else {
            egl_throw();
        }

        glClearColor(1.0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        if (eglSwapBuffers(egl_display, egl_window.surface())) {
            fmt::print("egl buffer swapped\n");
        } else {
            egl_throw();
        }
    } else {
        auto& my_shm = my_registry.get<wl::shm>();
        shm_buffer my_buffer {my_shm, win_width, win_height};
        my_surface.attach(my_buffer.buffer, 0, 0);

        std::fill (static_cast<std::uint32_t*>(my_buffer.pixels.get_address()),
                   static_cast<std::uint32_t*>(my_buffer.pixels.get_address()) + win_width * win_height,
                   (255 << 24) | (255 << 16) | (255 << 8) | (0 << 0)
        );

        my_surface.commit();
    }
    while (my_display.dispatch()) {
    }
    return 0;
}
