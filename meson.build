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

srm_dep     = dependency('SRM')
skia_dep    = dependency('Skia')
gl_dep      = dependency('gl')
egl_dep     = dependency('egl')
glesv2_dep  = dependency('glesv2')

executable(
    'srm-skia',
    sources : ['main.cpp'],
    dependencies : [
        srm_dep,
        skia_dep,
        egl_dep,
        glesv2_dep,
        egl_dep
    ],
    install : false)
