project('si', 'cpp', 'c', default_options: ['cpp_std=c++2a'])

includes = include_directories('include')

fmt = dependency('fmt')
wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
gl = dependency('gl')

executable('main',
    ['src/buffer.cpp', 'src/compositor.cpp', 'src/display.cpp', 'src/egl_window.cpp', 'src/egl.cpp', 'src/registry.cpp', 'src/shell.cpp', 'src/shell_surface.cpp', 'src/shm_buffer.cpp', 'src/shm.cpp', 'src/shm_pool.cpp', 'src/surface.cpp', 'src/wayland-client.cpp'],
    dependencies: [fmt, wayland_client, wayland_egl, egl, gl],
    include_directories: includes,
    link_args: '-lrt'
)
