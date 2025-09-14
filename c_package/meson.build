project('fprint-utilities', ['c'], version : '0.1')

include_directories = include_directories('/usr/include/libfprint-2')

libfprint_dep = dependency('libfprint-2', static: false, required: true)
json_glib_dep = dependency('json-glib-1.0')

dependencies = [
    libfprint_dep,
    json_glib_dep
]

sources = [
    'fingerprint.c',
    'utilities.c',
]

shared_library(
    'fp_reader',
    sources,
    dependencies: dependencies,
    install: true,
)

executable(
    'identify',
    sources,
    dependencies: dependencies
)
