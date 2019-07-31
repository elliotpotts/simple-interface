#ifndef SI_EGL_HPP_INCLUDED
#define SI_EGL_HPP_INCLUDED

#include <EGL/egl.h>
#include <tuple>

[[ noreturn ]] void egl_throw();
void describe_egl_config(EGLDisplay dpy, EGLConfig cfg);
std::tuple<EGLContext, EGLConfig> init_egl(EGLDisplay dpy);

#endif
