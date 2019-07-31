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
#include <GL/gl.h>
#include <array>
#include <unique_any.hpp>

using namespace std::string_literals;

constexpr int bytes_per_pixel = 4;
constexpr int win_width = 480;
constexpr int win_height = 360;

namespace wl {
    struct buffer_deleter {
        void operator()(wl_buffer* b) const;
    };
    class buffer {
        std::unique_ptr<wl_buffer, buffer_deleter> const hnd;
    public:
        explicit buffer(wl_buffer*);
        buffer(const buffer&) = delete;
        explicit operator wl_buffer*() const;
    };
}
void wl::buffer_deleter::operator()(wl_buffer* b) const {
    wl_buffer_destroy(b);
}
wl::buffer::buffer(wl_buffer* buffer) : hnd(buffer) {
    if (!hnd) {
        throw std::runtime_error("Can't create buffer from nullptr");
    }
}
wl::buffer::operator wl_buffer*() const {
    return hnd.get();
}

namespace wl {
    struct surface_deleter {
        void operator()(wl_surface*) const;
    };
    class surface {
        std::unique_ptr<wl_surface, surface_deleter> const hnd;
        static void dispatch_frame(void* data, wl_callback* callback, std::uint32_t callback_data);
        wl_callback* frame_callback = nullptr;
        wl_callback_listener frame_listener = { dispatch_frame };
        std::function<void(std::chrono::milliseconds)> frame_handler;
    public:
        explicit surface(wl_surface*);
        explicit operator wl_surface*() const;
        void attach(buffer& buf, std::int32_t x, std::int32_t y);
        void commit();
        template<typename F>
        void onframe(F&& f) {
            frame_handler = std::forward<F>(f);
            frame_callback = wl_surface_frame(hnd.get());
            wl_callback_add_listener(frame_callback, &frame_listener, this);
        }
    };
};
void wl::surface_deleter::operator()(wl_surface* sfc) const {
    wl_surface_destroy(sfc);
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
    return hnd.get();
}
void wl::surface::attach(wl::buffer& buf, std::int32_t x, std::int32_t y) {
    wl_surface_attach(hnd.get(), static_cast<wl_buffer*>(buf), x, y);
}
void wl::surface::commit() {
    wl_surface_commit(hnd.get());
}

namespace wl {
    struct compositor_deleter {
        void operator()(wl_compositor*) const;
    };
    class compositor {
        std::unique_ptr<wl_compositor, compositor_deleter> const hnd;
    public:
        explicit compositor(wl_compositor*);
        ~compositor() = default;
        surface make_surface();
    };
}
void wl::compositor_deleter::operator()(wl_compositor* comp) const {
    wl_compositor_destroy(comp);
}
wl::compositor::compositor(wl_compositor* compositor) : hnd(compositor) {
    if (!hnd) {
        throw std::runtime_error("Can't create compositor from nullptr!");
    }
}
wl::surface wl::compositor::make_surface() {
    return wl::surface{ wl_compositor_create_surface(hnd.get()) };
}

namespace wl {
    struct shell_surface_deleter {
        void operator()(wl_shell_surface*) const;
    };
    class shell_surface {
        std::unique_ptr<wl_shell_surface, shell_surface_deleter> const hnd;      
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
        ~shell_surface() = default;
        void set_toplevel();
    };
}
void wl::shell_surface_deleter::operator()(wl_shell_surface* sfc) const {
    wl_shell_surface_destroy(sfc);
}
wl::shell_surface::shell_surface(wl_shell_surface* surface) : hnd(surface) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell_surface from nullptr!\n");
    }
    wl_shell_surface_add_listener(hnd.get(), &listener, nullptr);
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
    wl_shell_surface_set_toplevel(hnd.get());
}

namespace wl {
    struct shell_deleter {
        void operator()(wl_shell*) const;
    };
    class shell {
        std::unique_ptr<wl_shell, shell_deleter> const hnd;
    public:
        explicit shell(wl_shell*);
        explicit operator const wl_shell*() const;
        shell_surface make_surface(const surface&);
    };
}
void wl::shell_deleter::operator()(wl_shell* shell) const {
    wl_shell_destroy(shell);
}
wl::shell::shell(wl_shell* shell) : hnd(shell) {
    if (!hnd) {
        throw std::runtime_error("Can't create shell from nullptr!");
    }
}
wl::shell_surface wl::shell::make_surface(const surface& surface) {
    return wl::shell_surface{ wl_shell_get_shell_surface(hnd.get(), static_cast<wl_surface*>(surface)) };
}

namespace wl {
    struct shm_pool_deleter {
        void operator()(wl_shm_pool*) const;
    };
    class shm_pool {
        std::unique_ptr<wl_shm_pool, shm_pool_deleter> const hnd;
    public:
        explicit shm_pool(wl_shm_pool*);
        explicit operator wl_shm_pool*() const;
        buffer make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format);
    };
}
void wl::shm_pool_deleter::operator()(wl_shm_pool* pool) const {
    wl_shm_pool_destroy(pool);
}
wl::shm_pool::shm_pool(wl_shm_pool* pool) : hnd(pool) {
    if (!hnd) {
        throw std::runtime_error("Can't create pool from nullptr!");
    }
}
wl::shm_pool::operator wl_shm_pool*() const {
    return hnd.get();
}
wl::buffer wl::shm_pool::make_buffer(std::int32_t offset, std::int32_t width, std::int32_t height, std::int32_t stride, std::int32_t format) {
    return wl::buffer{ wl_shm_pool_create_buffer(hnd.get(), offset, width, height, stride, format) };
}

namespace wl {
    struct shm_deleter {
        void operator()(wl_shm*) const;
    };
    class shm {
        std::unique_ptr<wl_shm, shm_deleter> const hnd;
        static void format(void* data, wl_shm* shm, std::uint32_t format);
        wl_shm_listener listener { format };
    public:
        explicit shm(wl_shm*);
        explicit operator wl_shm*() const;
        shm_pool make_pool(std::int32_t fd, std::int32_t size);
    };
}
void wl::shm_deleter::operator()(wl_shm* shm) const {
    wl_shm_destroy(shm);
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
    return hnd.get();
}
wl::shm_pool wl::shm::make_pool(std::int32_t fd, std::int32_t size) {
    return wl::shm_pool { wl_shm_create_pool(hnd.get(), fd, size) };
}

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
void wl::registry_deleter::operator()(wl_registry* reg) const {
    wl_registry_destroy(reg);
}
wl::registry::registry(wl_registry* registry) : hnd(registry) {
    if (!hnd) {
        throw std::runtime_error("Can't create registry from nullptr!");
    }
    wl_registry_add_listener(hnd.get(), &listener, this);
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
    return hnd.get();
}

namespace wl {
    struct display_deleter {
        void operator()(wl_display*) const;
    };
    class display {
        std::unique_ptr<wl_display, display_deleter> const hnd;
    public:
        display();
        registry make_registry();
        int dispatch();
        void roundtrip();
        EGLDisplay egl();
    };
};
void wl::display_deleter::operator()(wl_display* dpy) const {
    wl_display_disconnect(dpy);
}
wl::display::display() : hnd(wl_display_connect(nullptr)) {
    if (!hnd) {
        throw std::runtime_error("Can't connect to wayland");
    }
}
wl::registry wl::display::make_registry() {
    return wl::registry{wl_display_get_registry(hnd.get())};
}
int wl::display::dispatch() {
    int count = wl_display_dispatch(hnd.get());
    if (count == -1) {
        throw std::runtime_error("Dispatch failed!");
    } else {
        return count;
    }
}
void wl::display::roundtrip() {
    wl_display_roundtrip(hnd.get());
}

[[noreturn]] void egl_throw() {
    switch (eglGetError()) {
    case EGL_SUCCESS: throw std::runtime_error("failed successfully");
    case EGL_NOT_INITIALIZED: throw std::runtime_error("EGL_NOT_INITIALIZED");
    case EGL_BAD_ACCESS: throw std::runtime_error("EGL_BAD_ACCESS");
    case EGL_BAD_ALLOC: throw std::runtime_error("EGL_BAD_ALLOC");
    case EGL_BAD_ATTRIBUTE: throw std::runtime_error("EGL_BAD_ATTRIBUTE");
    case EGL_BAD_CONTEXT: throw std::runtime_error("EGL_BAD_CONTEXT");
    case EGL_BAD_CONFIG: throw std::runtime_error("EGL_BAD_CONFIG");
    case EGL_BAD_CURRENT_SURFACE: throw std::runtime_error("EGL_BAD_CURRENT_SURFACE");
    case EGL_BAD_DISPLAY: throw std::runtime_error("EGL_BAD_DISPLAY");
    case EGL_BAD_SURFACE: throw std::runtime_error("EGL_BAD_SURFACE");
    case EGL_BAD_MATCH: throw std::runtime_error("EGL_BAD_MATCH");
    case EGL_BAD_PARAMETER: throw std::runtime_error("EGL_BAD_PARAMETER");
    case EGL_BAD_NATIVE_PIXMAP: throw std::runtime_error("EGL_BAD_NATIVE_PIXMAP");
    case EGL_BAD_NATIVE_WINDOW: throw std::runtime_error("EGL_BAD_NATIVE_WINDOW");
    case EGL_CONTEXT_LOST: throw std::runtime_error("EGL_CONTEXT_LOST");
    default: throw std::runtime_error("unknown error");
    }
}

EGLDisplay wl::display::egl() {
    if (EGLDisplay dpy = eglGetDisplay(hnd.get()); dpy == EGL_NO_DISPLAY) {
        egl_throw();
    } else {
        return dpy;
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

void describe_config(EGLDisplay dpy, EGLConfig cfg) {
    EGLint val = 0;
    if (!eglGetConfigAttrib(dpy, cfg, EGL_ALPHA_SIZE, &val)) {
        egl_throw();
    }
    fmt::print("   EGL_ALPHA_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_ALPHA_MASK_SIZE, &val);
    fmt::print("   EGL_ALPHA_MASK_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BIND_TO_TEXTURE_RGB, &val);
    fmt::print("   EGL_BIND_TO_TEXTURE_RGB: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BIND_TO_TEXTURE_RGBA, &val);
    fmt::print("   EGL_BIND_TO_TEXTURE_RGBA: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BLUE_SIZE, &val);
    fmt::print("   EGL_BLUE_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BUFFER_SIZE, &val);
    fmt::print("   EGL_BUFFER_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_COLOR_BUFFER_TYPE, &val);
    fmt::print("   EGL_COLOR_BUFFER_TYPE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFIG_CAVEAT, &val);
    fmt::print("   EGL_CONFIG_CAVEAT: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFIG_ID, &val);
    fmt::print("   EGL_CONFIG_ID: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFORMANT, &val);
    fmt::print("   EGL_CONFORMANT: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_DEPTH_SIZE, &val);
    fmt::print("   EGL_DEPTH_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_GREEN_SIZE, &val);
    fmt::print("   EGL_GREEN_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_LEVEL, &val);
    fmt::print("   EGL_LEVEL: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_LUMINANCE_SIZE, &val);
    fmt::print("   EGL_LUMINANCE_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_WIDTH, &val);
    fmt::print("   EGL_MAX_PBUFFER_WIDTH: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_HEIGHT, &val);
    fmt::print("   EGL_MAX_PBUFFER_HEIGHT: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_PIXELS, &val);
    fmt::print("   EGL_MAX_PBUFFER_PIXELS: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_SWAP_INTERVAL, &val);
    fmt::print("   EGL_MAX_SWAP_INTERVAL: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MIN_SWAP_INTERVAL, &val);
    fmt::print("   EGL_MIN_SWAP_INTERVAL: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_RENDERABLE, &val);
    fmt::print("   EGL_NATIVE_RENDERABLE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &val);
    fmt::print("   EGL_NATIVE_VISUAL_ID: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_TYPE, &val);
    fmt::print("   EGL_NATIVE_VISUAL_TYPE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_RED_SIZE, &val);
    fmt::print("   EGL_RED_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_RENDERABLE_TYPE, &val);
    fmt::print("   EGL_RENDERABLE_TYPE: {} | {} | {} | {}\n",
               val & EGL_OPENGL_BIT ? "EGL_OPENGL_BIT" : "",
               val & EGL_OPENGL_ES_BIT ? "EGL_OPENGL_ES_BIT " : "",
               val & EGL_OPENGL_ES2_BIT ? "EGL_OPENGL_ES2_BIT" : "",
               val & EGL_OPENVG_BIT ? "EGL_OPENVG_BIT" : "");
    eglGetConfigAttrib(dpy, cfg, EGL_SAMPLE_BUFFERS, &val);
    fmt::print("   EGL_SAMPLE_BUFFERS: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_SAMPLES, &val);
    fmt::print("   EGL_SAMPLES: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_STENCIL_SIZE, &val);
    fmt::print("   EGL_STENCIL_SIZE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_SURFACE_TYPE, &val);
    fmt::print("   EGL_SURFACE_TYPE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_TYPE, &val);
    fmt::print("   EGL_TRANSPARENT_TYPE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_RED_VALUE, &val);
    fmt::print("   EGL_TRANSPARENT_RED_VALUE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_GREEN_VALUE, &val);
    fmt::print("   EGL_TRANSPARENT_GREEN_VALUE: {}\n", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_BLUE_VALUE, &val);
    fmt::print("   EGL_TRANSPARENT_BLUE_VALUE: {}\n", val);
}

std::tuple<EGLContext, EGLConfig> init_egl(EGLDisplay dpy) {
    EGLint major;
    EGLint minor;
    if (eglInitialize(dpy, &major, &minor)) {
        fmt::print("Initialized egl {}.{}\n", major, minor);
    } else {
        egl_throw();
    }

    EGLint total_config_count;
    if (!eglGetConfigs(dpy, nullptr, 0, &total_config_count)) {
        egl_throw();
    }
    fmt::print("{} configs available\n", total_config_count);

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLint matching_config_count;
    if (!eglChooseConfig(dpy, config_attribs, nullptr, 0, &matching_config_count)) {
        egl_throw();
    }
    std::vector<EGLConfig> matching_configs(matching_config_count);
    if (!eglChooseConfig(dpy, config_attribs, matching_configs.data(), matching_config_count, &matching_config_count)) {
        egl_throw();
    }
    fmt::print("{} configs matching\n", matching_config_count);

    if (matching_config_count > 0) {
        fmt::print("choosing first matching config\n");
        auto context_config = matching_configs[0];
        EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        auto context = eglCreateContext(dpy, context_config, EGL_NO_CONTEXT, context_attribs);
        if (context == EGL_NO_CONTEXT) {
            egl_throw();
        }
        return std::make_tuple(context, context_config);
    } else {
        throw std::runtime_error("No matching EGL configs found, cannot create api context");
    }
}

namespace wl {
    class egl_window {
        struct egl_window_deleter {
            void operator()(wl_egl_window* win) const {
                wl_egl_window_destroy(win);
            }
        };
        std::unique_ptr<wl_egl_window, egl_window_deleter> hnd;
        EGLSurface egl_surface;
    public:
        egl_window(EGLDisplay egl_display, EGLConfig egl_config, EGLContext egl_context, surface& from, int w, int h):
            hnd {wl_egl_window_create(static_cast<wl_surface*>(from), w, h)} {
            if (hnd.get() == EGL_NO_SURFACE) {
                hnd.release();
                egl_throw();
            }
            
            egl_surface = eglCreateWindowSurface(egl_display, egl_config, hnd.get(), nullptr);
            if (egl_surface == EGL_NO_SURFACE) {
                egl_throw();
            }

            if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
                fmt::print("egl made current.\n");
            } else {
                egl_throw();
            }

            glClearColor(1.0, 1.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            glFlush();

            if (eglSwapBuffers(egl_display, egl_surface)) {
                fmt::print("egl buffer swapped\n");
            } else {
                egl_throw();
            }
        }
    };
}


int main() {
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    my_display.roundtrip(); // wait for messages sent by server in order to register handlers for servicse
    auto& my_compositor = my_registry.get<wl::compositor>();    
    auto& my_shell = my_registry.get<wl::shell>();
    
    wl::surface my_surface = my_compositor.make_surface();
    wl::shell_surface my_shell_surface = my_shell.make_surface(my_surface);
    my_shell_surface.set_toplevel();

    auto egl_display = my_display.egl();
    auto [egl_context, egl_config] = init_egl(egl_display);
    describe_config(egl_display, egl_config);
    wl::egl_window egl_window(egl_display, egl_config, egl_context, my_surface, 600, 480);
    
    while (my_display.dispatch()) {
    }

    return 0;
}
