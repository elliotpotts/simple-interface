project('si', 'cpp', 'c', default_options: ['cpp_std=c++2a'])

includes = include_directories('include')

fmt = dependency('fmt')
wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
gl = dependency('gl')

executable('main',
    ['src/wayland-client.cpp'],
    dependencies: [fmt, wayland_client, wayland_egl, egl, gl],
    include_directories: includes,
    link_args: '-lrt'
)
