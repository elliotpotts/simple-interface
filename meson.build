project('si', 'cpp', 'c', default_options: ['cpp_std=c++2a'], version: '0.1')
cmake = import('cmake')
glslc = find_program('glslc')

add_project_arguments('-Wall', language: 'cpp')
includes = [include_directories('include')]

deps = [dependency('fmt')]
src = ['src/buffer.cpp', 'src/compositor.cpp', 'src/display.cpp', 'src/egl/display.cpp', 'src/egl.cpp', 'src/egl_window.cpp', 'src/registry.cpp', 'src/seat.cpp', 'src/shell.cpp', 'src/si/util.cpp', 'src/shell_surface.cpp', 'src/shm_buffer.cpp', 'src/vk_renderer.cpp', 'src/shm.cpp', 'src/shm_pool.cpp', 'src/surface.cpp', 'src/wayland-client.cpp', 'src/wl/keyboard.cpp', 'src/wl/pointer.cpp']

if get_option('support_vk').enabled()
  deps += dependency('vulkan')
  add_project_arguments('-DVK_USE_PLATFORM_WAYLAND_KHR', language: 'cpp')
endif

if get_option('support_gl').enabled()
  add_project_arguments('-DSI_GL_SUPPORTED', language: 'cpp')
  deps += [dependency('gl'), cmake.subproject('glad').dependency('glad')]
endif

if get_option('support_wl').enabled()
  deps += dependency('wayland-client')
  if get_option('support_gl').enabled()
    deps += [dependency('wayland-egl'), dependency('egl')]
  endif
endif

if get_option('support_X').enabled()
  deps += dependency('xcb')
  src += 'src/X/xcb_display.cpp'
  if get_option('support_gl').enabled()
    deps += dependency('x11')
    src += 'src/X/glx_display.cpp'
  endif
endif

if get_option('support_blend2d').enabled()
  blend2d = cmake.subproject('blend2d', required: get_option('support_blend2d'))
  deps += blend2d.dependency('blend2d')
endif

src += custom_target('vert', output : 'vert.spv', input : 'src/shader.vert', command : [glslc, '--target-env=vulkan1.0', '-c', '@INPUT@', '-o', '@OUTPUT@'])
src += custom_target('frag', output : 'frag.spv', input : 'src/shader.frag', command : [glslc, '--target-env=vulkan1.0', '-c', '@INPUT@', '-o', '@OUTPUT@'])

executable('main',
    src,
    dependencies: deps,
    include_directories: includes,
    link_args: '-lrt'
)
