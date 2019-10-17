#ifndef SI_WL_REGISTRY_HPP_INCLUDED
#define SI_WL_REGISTRY_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <tuple>
#include <string>
#include <string_view>
#include <fmt/format.h>

namespace wl {
    struct registry_deleter {
        void operator()(wl_registry*) const;
    };
    class registry {
        std::unique_ptr<wl_registry, registry_deleter> hnd;
        std::unordered_map<std::string, std::tuple<wl_registry*, std::uint32_t, std::uint32_t>> proto_ifaces;
        
        static void handler(void* data, wl_registry* registry, std::uint32_t id, const char* iface_name, std::uint32_t version);
        static void remover(void* data, wl_registry* registry, std::uint32_t id);
        wl_registry_listener listener { handler, remover };
        
    public:
        explicit registry(wl_registry*);
        explicit operator wl_registry*() const;

        template<typename T>
        T make(std::string_view name = T::wl_interface_name) {
            // when c++20 is implemented, construction of a std::string should be unnecessary
            auto it = proto_ifaces.find({name.begin(), name.end()});
            if (it == proto_ifaces.end()) {
                throw std::runtime_error(fmt::format("Can't find wayland interface with name \"{}\"", name));
            } else {
                auto [registry, id, version] = it->second;
                proto_ifaces.erase(it);
                return T {registry, id, version};
            }
        }
    };
}

#endif
