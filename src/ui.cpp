#include <si/ui.hpp>
#include <si/wl.hpp>

void si::run(const ::si::window& window) {
    //TODO: detect which renderer + window system is best to use
    si::wl_run(window);
}
