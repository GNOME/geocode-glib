project('geocode-glib', 'c', version: '3.26.4', meson_version : '>= 0.49.0')

gclib_version = meson.project_version() # set in project() below
ver_arr = gclib_version.split('.')

gclib_major_version = ver_arr[0]
gclib_minor_version = ver_arr[1]
gclib_micro_version = ver_arr[2]

if get_option('soup2')
  gclib_api_version='1.0'
else
  gclib_api_version='2.0'
endif

conf = configuration_data()
conf.set_quoted('VERSION', gclib_version)
conf.set_quoted('PACKAGE_VERSION', gclib_version)
conf.set_quoted('PACKAGE_NAME', 'geocode-glib')
conf.set_quoted('GETTEXT_PACKAGE', 'geocode-glib')
conf.set_quoted('PACKAGE_TARNAME', 'geocode-glib')
conf.set_quoted('PACKAGE_STRING', 'geocode-glib ' + gclib_version)
conf.set_quoted('PACKAGE_URL', 'https://developer.gnome.org/geocode-glib')
conf.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/geocode-glib/-/issues/')
conf.set_quoted('TEST_SRCDIR', meson.source_root() + '/data/')
datadir = get_option('prefix') + '/' + get_option('datadir')
conf.set_quoted('GEOCODE_LOCALEDIR', datadir + '/locale')

configure_file(output: 'config.h', configuration : conf)

gnome = import('gnome')

add_project_arguments('-Wall', language: 'c')
cc = meson.get_compiler('c')
if cc.get_id() == 'gcc'
    add_project_arguments('-Wchar-subscripts',
                          '-Wmissing-declarations',
                          '-Wmissing-prototypes',
                          '-Wnested-externs',
                          '-Wpointer-arith',
                          '-Wcast-align',
                          '-Wsign-compare',
                          '-Wredundant-decls',
                          '-fno-strict-aliasing',
                          language: 'c')
endif

subdir('geocode-glib')
subdir('po')
subdir('icons')

if get_option('enable-gtk-doc')
  subdir('docs')
endif
