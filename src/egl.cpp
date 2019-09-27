#include <si/egl.hpp>
#include <spdlog/spdlog.h>
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
    spdlog::info("   EGL_ALPHA_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_ALPHA_MASK_SIZE, &val);
    spdlog::info("   EGL_ALPHA_MASK_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BIND_TO_TEXTURE_RGB, &val);
    spdlog::info("   EGL_BIND_TO_TEXTURE_RGB: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BIND_TO_TEXTURE_RGBA, &val);
    spdlog::info("   EGL_BIND_TO_TEXTURE_RGBA: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BLUE_SIZE, &val);
    spdlog::info("   EGL_BLUE_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_BUFFER_SIZE, &val);
    spdlog::info("   EGL_BUFFER_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_COLOR_BUFFER_TYPE, &val);
    spdlog::info("   EGL_COLOR_BUFFER_TYPE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFIG_CAVEAT, &val);
    spdlog::info("   EGL_CONFIG_CAVEAT: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFIG_ID, &val);
    spdlog::info("   EGL_CONFIG_ID: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_CONFORMANT, &val);
    spdlog::info("   EGL_CONFORMANT: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_DEPTH_SIZE, &val);
    spdlog::info("   EGL_DEPTH_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_GREEN_SIZE, &val);
    spdlog::info("   EGL_GREEN_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_LEVEL, &val);
    spdlog::info("   EGL_LEVEL: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_LUMINANCE_SIZE, &val);
    spdlog::info("   EGL_LUMINANCE_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_WIDTH, &val);
    spdlog::info("   EGL_MAX_PBUFFER_WIDTH: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_HEIGHT, &val);
    spdlog::info("   EGL_MAX_PBUFFER_HEIGHT: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_PBUFFER_PIXELS, &val);
    spdlog::info("   EGL_MAX_PBUFFER_PIXELS: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MAX_SWAP_INTERVAL, &val);
    spdlog::info("   EGL_MAX_SWAP_INTERVAL: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_MIN_SWAP_INTERVAL, &val);
    spdlog::info("   EGL_MIN_SWAP_INTERVAL: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_RENDERABLE, &val);
    spdlog::info("   EGL_NATIVE_RENDERABLE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &val);
    spdlog::info("   EGL_NATIVE_VISUAL_ID: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_TYPE, &val);
    spdlog::info("   EGL_NATIVE_VISUAL_TYPE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_RED_SIZE, &val);
    spdlog::info("   EGL_RED_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_RENDERABLE_TYPE, &val);
    spdlog::info("   EGL_RENDERABLE_TYPE: {} | {} | {} | {}",
               val & EGL_OPENGL_BIT ? "EGL_OPENGL_BIT" : "",
               val & EGL_OPENGL_ES_BIT ? "EGL_OPENGL_ES_BIT " : "",
               val & EGL_OPENGL_ES2_BIT ? "EGL_OPENGL_ES2_BIT" : "",
               val & EGL_OPENVG_BIT ? "EGL_OPENVG_BIT" : "");
    eglGetConfigAttrib(dpy, cfg, EGL_SAMPLE_BUFFERS, &val);
    spdlog::info("   EGL_SAMPLE_BUFFERS: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_SAMPLES, &val);
    spdlog::info("   EGL_SAMPLES: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_STENCIL_SIZE, &val);
    spdlog::info("   EGL_STENCIL_SIZE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_SURFACE_TYPE, &val);
    spdlog::info("   EGL_SURFACE_TYPE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_TYPE, &val);
    spdlog::info("   EGL_TRANSPARENT_TYPE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_RED_VALUE, &val);
    spdlog::info("   EGL_TRANSPARENT_RED_VALUE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_GREEN_VALUE, &val);
    spdlog::info("   EGL_TRANSPARENT_GREEN_VALUE: {}", val);
    eglGetConfigAttrib(dpy, cfg, EGL_TRANSPARENT_BLUE_VALUE, &val);
    spdlog::info("   EGL_TRANSPARENT_BLUE_VALUE: {}", val);
}

std::tuple<EGLContext, EGLConfig> init_egl(EGLDisplay dpy) {
    EGLint major;
    EGLint minor;
    if (eglInitialize(dpy, &major, &minor)) {
        spdlog::info("Initialized egl {}.{}", major, minor);
    } else {
        egl_throw();
    }

    EGLint total_config_count;
    if (!eglGetConfigs(dpy, nullptr, 0, &total_config_count)) {
        egl_throw();
    }
    spdlog::info("{} configs available", total_config_count);

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
    spdlog::info("{} configs matching", matching_config_count);

    if (matching_config_count > 0) {
        spdlog::info("choosing first matching config");
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
