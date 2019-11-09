#include <si/wl.hpp>
#include <si/wl/display.hpp>
#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shm.hpp>
#include <si/wl/seat.hpp>
#include <si/wlp/xdg_shell.hpp>
#include <si/vk_renderer.hpp>
#include <si/ui.hpp>

void si::wl_run(const ::si::window& win) {
    ::wl::display my_display;
    ::wl::registry my_registry = my_display.make_registry();
    
    my_display.roundtrip(); // roundtrip to allow registry making events to be processed
    auto my_compositor = my_registry.make<::wl::compositor>();
    auto my_seat = my_registry.make<si::wl::seat>("wl_seat");
    auto my_ptr = my_seat.pointer();
    auto my_keyboard = my_seat.keyboard();
    auto my_wm_base = my_registry.make<si::wlp::xdg_wm_base>("xdg_wm_base");
    my_wm_base.on_ping.connect (
        [&](std::uint32_t serial) {
            my_wm_base.pong(serial);
        }
    );
        
    ::wl::surface my_surface = my_compositor.make_surface();
    si::wlp::xdg_surface my_xdg_surface = my_wm_base.get_xdg_surface(static_cast<wl_surface*>(my_surface));
    si::wlp::xdg_toplevel my_xdg_toplevel = my_xdg_surface.get_toplevel();
    my_xdg_toplevel.set_app_id("Simple Interface");
    my_surface.commit();
    int new_width;
    int new_height;
    my_xdg_toplevel.on_configure.connect(
        [&](int width, int height, wl_array* states) {
            new_width = width;
            new_height = height;
        }
    );

    // Use vulkan renderer for now
    si::vk::root vk;
    auto r = vk.make_renderer(my_display, my_surface, win.width, win.height);
    my_xdg_surface.on_configure.connect(
        [&](std::uint32_t serial) {
            if (new_width != 0 && new_height != 0) {
                r->resize(new_width, new_height);
                my_xdg_surface.ack_configure(serial);
                my_surface.commit();
            }
        }
    );
    while (my_display.dispatch() != -1) {
        r->draw();
    }
}
