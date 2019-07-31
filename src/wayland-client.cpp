#include <fmt/format.h>

#include <si/wl/display.hpp>
#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/egl.hpp>
#include <si/wl/egl_window.hpp>

#include <GL/gl.h>

int main() {
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    my_display.roundtrip(); // wait for messages sent by server in order to register handlers for servicse
    auto& my_compositor = my_registry.get<wl::compositor>();    
    auto& my_shell = my_registry.get<wl::shell>();
    
    wl::surface my_surface = my_compositor.make_surface();
    wl::shell_surface my_shell_surface = my_shell.make_surface(my_surface);
    my_shell_surface.set_toplevel();

    EGLDisplay egl_display = my_display.egl();
    auto [egl_context, egl_config] = init_egl(egl_display);
    describe_egl_config(egl_display, egl_config);
    wl::egl_window egl_window(egl_display, egl_config, egl_context, my_surface, 600, 480);

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
    /* Done painting, just leave it open now. */
    
    while (my_display.dispatch()) {
    }

    return 0;
}
