header_subdir = 'geocode-glib-' + gclib_api_version + '/geocode-glib'
header_dir = get_option('includedir') + '/' + header_subdir

headers = [ 'geocode-glib.h',
            'geocode-location.h',
            'geocode-forward.h',
            'geocode-reverse.h',
            'geocode-error.h',
            'geocode-place.h',
            'geocode-bounding-box.h',
            'geocode-backend.h',
            'geocode-mock-backend.h',
            'geocode-nominatim.h' ]

generated_sources = gnome.mkenums('geocode-enum-types',
                                  h_template: 'geocode-enum-types.h.in',
                                  c_template: 'geocode-enum-types.c.in',
                                  sources: headers,
                                  install_header: true,
                                  install_dir: header_dir)


public_sources = [ 'geocode-location.c',
                   'geocode-forward.c',
                   'geocode-reverse.c',
                   'geocode-glib.c',
                   'geocode-error.c',
                   'geocode-place.c',
                   'geocode-bounding-box.c',
                   'geocode-backend.c',
                   'geocode-mock-backend.c',
                   'geocode-nominatim.c' ] + generated_sources

sources = public_sources + [ 'geocode-glib-private.h' ]

deps = [ dependency('gio-2.0', version: '>= 2.34'),
		 dependency('json-glib-1.0', version: '>= 0.99.2'),
		 dependency('libsoup-2.4', version: '>= 2.42') ]
libm = cc.find_library('m', required: false)
if libm.found()
    deps += [ libm ]
endif

include = include_directories('..')
gclib_map = join_paths(meson.current_source_dir(), 'geocode-glib.map')

libgcglib = shared_library('geocode-glib',
                           sources,
                           dependencies: deps,
                           include_directories: include,
                           link_depends: gclib_map,
                           link_args: [ '-Wl,--version-script,' + gclib_map ],
                           soversion: '0',
                           version: '0.0.0',
                           install: true)

install_headers(headers, subdir: header_subdir)

pkgconf = import('pkgconfig')
pkgconf.generate(libraries: libgcglib,
                 version: gclib_version,
                 name: 'geocode-glib',
                 description: 'Helper library for geocoding services',
                 filebase: 'geocode-glib-@0@'.format(gclib_api_version),
                 subdirs: 'geocode-glib-@0@'.format(gclib_api_version),
                 requires: 'gio-2.0',
                 variables: [
                   'apiversion=' + gclib_api_version,
                 ])

gir = find_program('g-ir-scanner', required: false)
cross_build = meson.is_cross_build()
enable_gir = get_option('enable-introspection')

if gir.found() and not cross_build and enable_gir
  gir_args = [
    '--quiet',
	'--c-include=geocode-glib/geocode-glib.h'
  ]

  gnome.generate_gir(libgcglib,
                     sources: public_sources + headers,
                     namespace: 'GeocodeGlib',
                     nsversion: gclib_api_version,
                     identifier_prefix: 'Geocode',
                     symbol_prefix: 'geocode',
                     export_packages: 'geocode-glib-1.0',
                     dependencies: deps,
                     includes: [ 'GObject-2.0',
                                 'Gio-2.0',
                                 'Json-1.0',
                                 'Soup-2.4' ],
                     install: true,
                     extra_args: gir_args)
endif

geocode_glib_dep = declare_dependency(link_with: libgcglib,
                                      include_directories: include,
                                      dependencies: deps,
                                      sources: generated_sources)

if get_option('enable-installed-tests')
    subdir('tests')
endif
