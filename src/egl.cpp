#include <si/egl.hpp>
#include <fmt/format.h>
#include <stdexcept>
#include <vector>

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

void describe_egl_config(EGLDisplay dpy, EGLConfig cfg) {
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
