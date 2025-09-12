
project('fprint-examples', 'c', version : '0.1')

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
gobject_dep = dependency('gobject-2.0')
libfprint_dep = dependency('libfprint-2')
json_glib_dep = dependency('json-glib-1.0')

# Lista de todos los módulos a incluir en la biblioteca compartida
sources = [
  'identify.c',
  'verify.c',
  'enroll.c',
  'storage.c',
  'utilities.c',
]

shared_library(
  'fp_reader',
  sources,
  dependencies: [libfprint_dep, glib_dep, json_glib_dep],
  install: true,
)
