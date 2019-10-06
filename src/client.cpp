#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
using namespace std::literals::string_literals;
#include <si/util.hpp>

#include <si/wl/display.hpp>
#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/wl/shm.hpp>
#include <si/wl/seat.hpp>
#include <si/wlp/xdg_shell.hpp>

#include <si/egl.hpp>
#include <si/wl/egl_window.hpp>
#include <si/shm_buffer.hpp>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <si/vk_renderer.hpp>

enum draw_method {
    draw_opengl,
    draw_gles,
    draw_vulkan,
    draw_shm
};
namespace {
    const int win_width = 480;
    const int win_height = 360;
}
auto start_time = std::chrono::system_clock::now();

int main() {
    spdlog::set_level(spdlog::level::debug);
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    //my_display.dispatch(); // TODO: Figure out when I need to dispatch/roundtrip
    my_display.roundtrip();

    auto& my_compositor = my_registry.get<wl::compositor>();
    auto& my_seat = my_registry.get<si::wl::seat>();
    auto my_ptr = my_seat.pointer();
    auto my_keyboard = my_seat.keyboard();
    auto& my_wm_base = my_registry.get<si::wlp::xdg_wm_base>();
    my_wm_base.on_ping.connect([&](std::uint32_t serial) {
                                   my_wm_base.pong(serial);
                               });

    wl::surface my_surface = my_compositor.make_surface();
    /* 0. 
     * 4. Client recieves configure event
     * 5. Client ack_configure's if it indends to eventually commit
     * 6. Client commits
     */
    si::wlp::xdg_surface my_xdg_surface = my_wm_base.get_xdg_surface(static_cast<wl_surface*>(my_surface));
    si::wlp::xdg_toplevel my_xdg_toplevel = my_xdg_surface.get_toplevel();
    my_xdg_toplevel.set_app_id("Simple Interface");
    my_surface.commit();
    int new_width;
    int new_height;
    my_xdg_toplevel.on_configure.connect([&](int width, int height, wl_array* states) {
                                             new_width = width;
                                             new_height = height;
                                         });

    draw_method draw_with = draw_vulkan;
    if (draw_with == draw_vulkan) {
        si::vk::root vk;
        auto r = vk.make_renderer(my_display, my_surface, 230, 230);
        my_xdg_surface.on_configure.connect([&](std::uint32_t serial) {
                                                if (new_width != 0 && new_height != 0) {
                                                    spdlog::debug("Resizing to {}x{}", new_width, new_height);
                                                    my_xdg_surface.ack_configure(serial);
                                                    r->resize(new_width, new_height);
                                                    spdlog::debug("Committing...");
                                                    my_surface.commit();
                                                }
                                            });
        r->draw();
        while (my_display.dispatch() != -1) {
        }
    } else if (draw_with == draw_opengl) {
        EGLDisplay egl_display = my_display.egl();
        auto [egl_context, egl_config] = init_egl(egl_display);
        describe_egl_config(egl_display, egl_config);
        wl::egl_window egl_window(egl_display, egl_config, egl_context, my_surface, win_width, win_height);

        /* Paint on the freshly created rendering context */
        if (eglMakeCurrent(egl_display, egl_window.surface(), egl_window.surface(), egl_context)) {
            spdlog::info("egl made current.");
        } else {
            egl_throw();
        }

        for (unsigned i = 0; i < 1; i++) {
            glClearColor(1.0, 1.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            glFlush();          
            if (eglSwapBuffers(egl_display, egl_window.surface())) {
                spdlog::info("egl buffer swapped");
            } else {
                egl_throw();
            }
        }
    } else if (draw_with == draw_shm) {
        auto& my_shm = my_registry.get<wl::shm>();
        shm_buffer my_buffer {my_shm, win_width, win_height};
        my_surface.attach(my_buffer.buffer, 0, 0);

        std::fill (static_cast<std::uint32_t*>(my_buffer.pixels.get_address()),
                   static_cast<std::uint32_t*>(my_buffer.pixels.get_address()) + win_width * win_height,
                   (255 << 24) | (255 << 16) | (255 << 8) | (0 << 0)
        );

        my_surface.commit();
    } else {
        spdlog::info("unknown draw method");
    }
    while (my_display.dispatch() != -1) {
    }
    spdlog::info("finished");
    return 0;
}
