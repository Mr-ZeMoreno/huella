project('fprint-examples', ['c', 'cpp'],
  version: '0.1',
  default_options: ['cpp_std=c++17']
)

# Dependencias
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
gobject_dep = dependency('gobject-2.0')
libfprint_dep = dependency('libfprint-2')
json_glib_dep = dependency('json-glib-1.0')

# Lista de ejemplos en C
examples = [
  'enroll',
  'identify',
  'img-capture',
  'manage-prints',
  'verify',
  'clear-storage',
]

# Para cada ejemplo C, crea un ejecutable
foreach example : examples
  executable(
    example,
    [ example + '.c', 'storage.c', 'utilities.c' ],
    dependencies: [libfprint_dep, glib_dep, json_glib_dep],
  )
endforeach

# Ejecutable en C++
executable(
  'cpp-test',
  'cpp-test.cpp',
  dependencies: [libfprint_dep, glib_dep, gio_dep, gobject_dep],
)
