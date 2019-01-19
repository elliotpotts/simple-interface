#include <cstdint>
#include <cerrno>
#include <string>
#include <forward_list>
#include <utility>
#include <memory>
#include <fmt/format.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/signals2.hpp>
#include <chrono>
#include <functional>
#include <cmath>
#include <EGL/egl.h>
#include <array>

class unique_any {
    struct contents {
        virtual ~contents() = default;
    };
    
    template<typename T>
    struct typed_contents : public contents {
        T storage;
        template<typename... Args>
        typed_contents(Args&&... args) : storage{ std::forward<Args>(args)... } {
        }
        virtual ~typed_contents() = default;
    };
    
    std::unique_ptr<contents> storage;
public:
    template<typename T, typename... Args>
    unique_any(std::in_place_type_t<T>, Args&&... args) :
        storage(std::make_unique<typed_contents<T>>(std::forward<Args>(args)...)) {
    }
    
    template<typename T>
    T* get() {
        auto ptr = dynamic_cast<typed_contents<T>*>(storage.get());
        if (ptr) {
            return &ptr->storage;
        } else {
            return nullptr;
        }
    }
};

using namespace std::string_literals;

constexpr int bytes_per_pixel = 4;
constexpr int win_width = 480;
constexpr int win_height = 360;

namespace wl {
    class buffer {
        wl_buffer* const hnd;
    public:
        explicit buffer(wl_buffer*);
        buffer(const buffer&) = delete;
        explicit operator wl_buffer*() const;
    };
}
wl::buffer::buffer(wl_buffer* buffer) : hnd{buffer} {
    if (!hnd) {
        throw std::runtime_error("Can't create buffer from nullptr");
    }
}
wl::buffer::operator wl_buffer*() const {
    return hnd;
}

namespace wl {
    class surface {
        wl_surface* const hnd;
        static void dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data);
        wl_callback* frame_callback = nullptr;
        wl_callback_listener frame_listener = { dispatch_frame };
        std::function<void(std::chrono::milliseconds)> frame_handler;
    public:
        explicit surface(wl_surface*);
        surface(const surface&) = delete;
        explicit operator wl_surface*() const;
        void attach(buffer& buf, std::int32_t x, std::int32_t y);
        void commit();
        template<typename F>
        void onframe(F&& f) {
            frame_handler = std::forward<F>(f);
            frame_callback = wl_surface_frame(hnd);
            wl_callback_add_listener(frame_callback, &frame_listener, this);
        }
    };
};
wl::surface::surface(wl_surface* surface) : hnd(surface) {
    if (!hnd) {
        throw std::runtime_error("Can't create surface from nullptr!");
    }
}
void wl::surface::dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data) {
    auto& surface = *reinterpret_cast<wl::surface*>(data);
    surface.frame_handler(std::chrono::milliseconds{callback_data});
}
wl::surface::operator wl_surface*() const {
    return hnd;
}
void wl::surface::attach(wl::buffer& buf, std::int32_t x, std::int32_t y) {
    wl_surface_attach(hnd, static_cast<wl_buffer*>(buf), x, y);
}
void wl::surface::commit() {
    wl_surface_commit(hnd);
}

namespace wl {    
    class compositor {
        wl_compositor* const hnd;
    public:
        explicit compositor(wl_compositor*);
        compositor(const compositor&) = delete;
        surface make_surface();
    };
}
wl::compositor::compositor(wl_compositor* compositor) : hnd(compositor) {
    if (!hnd) {
        throw std::runtime_error("Can't create compositor from nullptr!");
    }
}
wl::surface wl::compositor::make_surface() {
    return wl::surface{ wl_compositor_create_surface(hnd) };
}

namespace wl {
    class shell_surface {
        wl_shell_surface* const hnd;      
        static void handle_ping(void*, wl_shell_surface*, std::uint32_t);          
        static void handle_configure(void*, wl_shell_surface*, std::uint32_t, std::int32_t, std::int32_t);
        static void handle_popup_done(void*, wl_shell_surface*);
        const wl_shell_surface_listener listener = {
            handle_ping,
            handle_configure,
            handle_popup_done
        };
    public:
        explicit shell_surface(wl_shell_surface*);
        shell_surface(const shell_surface&) = delete;
        void set_toplevel();
    };
}
wl::shell_surface::shell_surface(wl_shell_surface* surface) : hnd(surface) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell_surface from nullptr!\n");
    }
    wl_shell_surface_add_listener(hnd, &listener, nullptr);
}
void wl::shell_surface::handle_ping(void* data, wl_shell_surface* shell_surface, std::uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
    fmt::print("Pinged and ponged\n");
}
void wl::shell_surface::handle_configure(void*, wl_shell_surface*, std::uint32_t, std::int32_t, std::int32_t) {
}
void wl::shell_surface::handle_popup_done(void*, wl_shell_surface*) {
}
void wl::shell_surface::set_toplevel() {
    wl_shell_surface_set_toplevel(hnd);
}

namespace wl {
    class shell {
        wl_shell* const hnd;
    public:
        explicit shell(wl_shell*);
        shell(const shell&) = delete;
        ~shell();
        explicit operator const wl_shell*() const;
        shell_surface make_surface(const surface&);
    };
}
wl::shell::shell(wl_shell* shell) : hnd(shell) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell from nullptr!");
    }
}
wl::shell::~shell() {
    fmt::print("destroying shell\n");
    wl_shell_destroy(hnd);
}
wl::shell_surface wl::shell::make_surface(const surface& surface) {
    return wl::shell_surface{ wl_shell_get_shell_surface(hnd, static_cast<wl_surface*>(surface)) };
}

namespace wl {
    class shm_pool {
        wl_shm_pool* const hnd;
    public:
        explicit shm_pool(wl_shm_pool*);
        shm_pool(const shm_pool&) = delete;
        ~shm_pool();
        explicit operator wl_shm_pool*() const;
        buffer make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format);
    };
}
wl::shm_pool::shm_pool(wl_shm_pool* pool) : hnd(pool) {
    if (!hnd) {
        throw std::runtime_error("Can't create pool from nullptr!");
    }
}
wl::shm_pool::operator wl_shm_pool*() const {
    return hnd;
}
wl::buffer wl::shm_pool::make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format) {
    return wl::buffer{ wl_shm_pool_create_buffer(hnd, offset, width, height, stride, format) };
}
wl::shm_pool::~shm_pool() {
    wl_shm_pool_destroy(hnd);
}

namespace wl {
    class shm {
        wl_shm* const hnd;
        static void format(void* data, wl_shm* shm, std::uint32_t format);
        wl_shm_listener listener { format };
    public:
        explicit shm(wl_shm*);
        explicit operator wl_shm*() const;
        shm_pool make_pool(std::int32_t fd, std::int32_t size);
    };
}
wl::shm::shm(wl_shm* shm) : hnd(shm) {
    if (!hnd) {
        throw std::runtime_error("Can't create shm from nullptr!");
    }
    wl_shm_add_listener(shm, &listener, nullptr);
}
void wl::shm::format(void* data, wl_shm* shm, std::uint32_t format) {
    fmt::print("Format {}\n", format);
}
wl::shm::operator wl_shm*() const {
    return hnd;
}
wl::shm_pool wl::shm::make_pool(std::int32_t fd, std::int32_t size) {
    return wl::shm_pool { wl_shm_create_pool(hnd, fd, size) };
}

namespace wl {
    class registry {
        wl_registry* hnd;
        std::forward_list<unique_any> objects;
        static void handler(void* data, wl_registry* registry, std::uint32_t id, const char* iface_name, std::uint32_t version);
        static void remover(void* data, wl_registry* registry, std::uint32_t id);
        wl_registry_listener listener { handler, remover };
        
    public:
        explicit registry(wl_registry*);
        registry(const registry&) = delete;
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
wl::registry::registry(wl_registry* registry) : hnd{registry} {
    if (!hnd) {
        throw std::runtime_error("Can't create registry from nullptr!");
    }
    wl_registry_add_listener(hnd, &listener, this);
}
void wl::registry::handler(void* data, wl_registry* registry, std::uint32_t id, const char* iface, std::uint32_t version) {
    auto& reg = *reinterpret_cast<wl::registry*>(data);
    auto reg_ptr = static_cast<wl_registry*>(reg);
    if (iface == "wl_compositor"s) {
        reg.objects.emplace_front (
            std::in_place_type<compositor>,
            static_cast<wl_compositor*> (wl_registry_bind(reg_ptr, id, &wl_compositor_interface, version))
        );
    } else if (iface == "wl_shell"s) {
        reg.objects.emplace_front (
            std::in_place_type<shell>,
            static_cast<wl_shell*> (wl_registry_bind(reg_ptr, id, &wl_shell_interface, version))
        );
    } else if (iface == "wl_shm"s) {
        reg.objects.emplace_front (
            std::in_place_type<shm>,
            static_cast<wl_shm*> (wl_registry_bind(reg_ptr, id, &wl_shm_interface, version))
        );
    }
    else {
        fmt::print("{:>3}: {}: no handler\n", id, iface);
        return;
    }
    fmt::print("{:>3}: {}: bound successfully\n", id, iface);
}
void wl::registry::remover(void* data, wl_registry* registry, std::uint32_t id) {
    fmt::print("Got a registry losing event for id {}\n", id);
}
wl::registry::operator wl_registry*() const {
    return hnd;
}

namespace wl {
    class display {
        wl_display* const hnd;
    public:
        display();
        ~display();
        registry make_registry();
        int dispatch();
        void roundtrip();
        EGLDisplay egl();
    };
};
wl::display::display() : hnd(wl_display_connect(nullptr)) {
    if (!hnd) {
        throw std::runtime_error("Can't connect to wayland");
    }
}
wl::display::~display() {
    fmt::print("!!!!Disconnecting\n");
    wl_display_disconnect(hnd);
}
wl::registry wl::display::make_registry() {
    return wl::registry{wl_display_get_registry(hnd)};
}
int wl::display::dispatch() {
    int count = wl_display_dispatch(hnd);
    if (count == -1) {
        throw std::runtime_error("Dispatch failed!");
    } else {
        return count;
    }
}
void wl::display::roundtrip() {
    wl_display_roundtrip(hnd);
}

EGLDisplay wl::display::egl() {
    EGLDisplay dpy = eglGetDisplay(hnd);
    if (hnd == EGL_NO_DISPLAY) {
        throw std::runtime_error("Can't get egl display\n");
    } else {
        return dpy;
    }
}

EGLContext init_egl(EGLDisplay dpy) {
    EGLint major;
    EGLint minor;
    EGLBoolean initialized = eglInitialize(dpy, &major, &minor);
    if (initialized == EGL_BAD_DISPLAY) {
        throw std::runtime_error("EGL initialization failed (EGL_BAD_DISPLAY)");
    } else if (initialized == EGL_NOT_INITIALIZED) {
        throw std::runtime_error("EGL initialization failed (EGL_NOT_INITIALIZED)");
    } else if (initialized == EGL_FALSE) {
        throw std::runtime_error("EGL initialization failed (unknown reason)");
    }
    else {
        fmt::print("Initialized egl {}.{}\n", major, minor);
    }

    EGLint total_config_count;
    eglGetConfigs(dpy, nullptr, 0, &total_config_count);
    std::vector<EGLConfig> all_configs(total_config_count);
    eglGetConfigs(dpy, all_configs.data(), total_config_count, nullptr);
    fmt::print("{} configs available\n", total_config_count);

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLint matching_config_count;
    eglChooseConfig(dpy, config_attribs, nullptr, 0, &matching_config_count);
    std::vector<EGLConfig> matching_configs(matching_config_count);
    eglChooseConfig(dpy, config_attribs, matching_configs.data(), matching_config_count, nullptr);
    fmt::print("{} configs matching\n", matching_config_count);

    if (matching_config_count > 0) {
        fmt::print("choosing 1st config\n");
        EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        return eglCreateContext(dpy, matching_configs[0], EGL_NO_CONTEXT, context_attribs);
    } else {
        throw std::runtime_error("No matching EGL configs found, cannot create api context");
    }
}

namespace ipc = boost::interprocess;
class shm_backed_buffer {
    static ipc::shared_memory_object make_shm_obj(int width, int height);
public:
    ipc::shared_memory_object shm_obj;
    ipc::mapped_region pixels;
    wl::shm_pool pool;
    wl::buffer buffer;
    shm_backed_buffer(wl::shm&, int width, int height);
};
ipc::shared_memory_object shm_backed_buffer::make_shm_obj(int width, int height) {
    ipc::shared_memory_object shm_obj {
        ipc::open_or_create,
        "foobar",
        ipc::read_write
    };
    shm_obj.truncate(width * height * bytes_per_pixel);
    return shm_obj;
}
shm_backed_buffer::shm_backed_buffer(wl::shm& shm_iface, int width, int height) :
    shm_obj{make_shm_obj(width, height)},
    pixels{shm_obj, ipc::read_write},
    pool{shm_iface.make_pool(shm_obj.get_mapping_handle().handle, pixels.get_size())},
    buffer(pool.make_buffer(0, width, height, width * bytes_per_pixel, WL_SHM_FORMAT_XRGB8888))
    {
}

int main() {
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    my_display.roundtrip(); // wait until they're processed by the server, or something :V
    init_egl(my_display.egl());

    auto& my_compositor = my_registry.get<wl::compositor>();
    wl::surface my_surface = my_compositor.make_surface();
    
    auto& my_shell = my_registry.get<wl::shell>();
    wl::shell_surface my_shell_surface = my_shell.make_surface(my_surface);
    my_shell_surface.set_toplevel();
    
    auto& my_shm = my_registry.get<wl::shm>();

    shm_backed_buffer my_buffer {my_shm, win_width, win_height};
    my_surface.attach(my_buffer.buffer, 0, 0);

    std::function<void()> attach_paint_again;
    auto paint =
        [&](std::chrono::milliseconds ms) {
            fmt::print("painting\n");
            std::fill(
                static_cast<int*>(my_buffer.pixels.get_address()),
                static_cast<int*>(my_buffer.pixels.get_address()) + win_width * win_height,
                static_cast<int>(std::sin(ms.count()) * 0x00ff00)
            );
            wl_surface_damage_buffer(static_cast<wl_surface*>(my_surface), 0, 0, win_width, win_height);
            attach_paint_again();
            my_surface.commit();
        };
    attach_paint_again =
        [&](){
            my_surface.onframe(paint);
        };
    
    my_surface.onframe(paint); //The frame request will take effect on the next wl_surface.commit.
    my_surface.commit();

    while (my_display.dispatch()) {
        fmt::print("d'd\n");
    }

    return 0;
}
