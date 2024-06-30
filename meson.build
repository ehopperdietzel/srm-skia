project(
    'SRM Skia',
    'c','cpp',
    version : '0.1.0',
    meson_version: '>= 0.62.0',
    default_options: [
        'warning_level=2',
        'buildtype=release',
        'cpp_std=c++20'
        ]
)

add_project_arguments([
    '-DSK_GL',
    '-DSK_GANESH'
], language: 'cpp')

cxx = meson.get_compiler('cpp')

srm_dep     = dependency('SRM')
skia_dep    = cxx.find_library('skia', dirs : meson.current_source_dir() + '/skia-canvaskit-0.34.0/out/Shared')
gl_dep      = dependency('gl')
egl_dep     = dependency('egl')
glesv2_dep  = dependency('glesv2')

executable(
    'srm-skia',
    sources : ['main.cpp'],
    include_directories : include_directories('./skia-canvaskit-0.34.0'),
    dependencies : [
        srm_dep,
        skia_dep,
        egl_dep,
        glesv2_dep,
        egl_dep
    ],
    install : false)
