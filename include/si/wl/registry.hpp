#ifndef SI_WL_REGISTRY_HPP_INCLUDED
#define SI_WL_REGISTRY_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>
#include <forward_list>
#include <cstdint>
#include <unique_any.hpp>

namespace wl {
    struct registry_deleter {
        void operator()(wl_registry*) const;
    };
    class registry {
        std::unique_ptr<wl_registry, registry_deleter> hnd;
        std::forward_list<unique_any> objects;
        static void handler(void* data, wl_registry* registry, std::uint32_t id, const char* iface_name, std::uint32_t version);
        static void remover(void* data, wl_registry* registry, std::uint32_t id);
        wl_registry_listener listener { handler, remover };
        
    public:
        explicit registry(wl_registry*);
        explicit operator wl_registry*() const;
        
        template<typename T>
        T& get() {
            for (auto& object : objects) {
                T* ptr = object.get<T>();
                if (ptr != nullptr) {
                    return *ptr;
                }
            }
            throw std::runtime_error("No object of given type!\n");
        }
    };
}

#endif
