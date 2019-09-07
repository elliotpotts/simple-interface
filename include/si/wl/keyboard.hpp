#ifndef SI_WL_KEYBOARD_HPP_INCLUDED
#define SI_WL_KEYBOARD_HPP_INCLUDED

#include <wayland-client-protocol.h>
#include <memory>

namespace si::wl {
    struct keyboard_deleter {
        void operator()(wl_keyboard*) const;
    };
    class keyboard {
        std::unique_ptr<wl_keyboard, keyboard_deleter> hnd;
        static void keymap(void* data, wl_keyboard* keyboard, std::uint32_t format, std::int32_t fd, std::uint32_t size);
        static void enter(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface, wl_array* keys);
        static void leave(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface);
        static void key(void* data, wl_keyboard* keyboard, std::uint32_t serial, std::uint32_t time, std::uint32_t key, std::uint32_t state);
        static void modifiers(void* data, wl_keyboard* keyboard, std::uint32_t serial, std::uint32_t mods_depressed, std::uint32_t mods_latched, std::uint32_t mods_locked, std::uint32_t group);
        static void repeat_info(void* data, wl_keyboard* keyboard, std::int32_t rate, std::int32_t delay);
        const wl_keyboard_listener listeners = { keymap, enter, leave, key, modifiers, repeat_info };
    public:
        explicit keyboard(wl_keyboard*);
    };    
}

#endif
