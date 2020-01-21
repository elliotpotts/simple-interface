#include <spdlog/spdlog.h>
#include <si/ui.hpp>

int main() {
    spdlog::set_level(spdlog::level::debug);
    
    si::window win { 832, 794 };

    si::run(win);
    return 0;
}
