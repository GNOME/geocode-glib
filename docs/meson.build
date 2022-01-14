if get_option('enable-gtk-doc') and not enable_gir
  error('API reference requires introspection.')
endif

ent_conf = configuration_data()
ent_conf.set('PACKAGE', 'Geocode-Glib')
ent_conf.set('PACKAGE_BUGREPORT', 'https://bugzilla.gnome.org/enter_bug.cgi?product=geocode-glib')
ent_conf.set('PACKAGE_NAME', 'Geocode-Glib')
ent_conf.set('PACKAGE_STRING', meson.project_name())
ent_conf.set('PACKAGE_TARNAME', '@0@-@1@'.format(meson.project_name(), meson.project_version()))
ent_conf.set('PACKAGE_URL', 'http://wiki.gnome.org/Project/GeocodeGlib')
ent_conf.set('PACKAGE_VERSION', meson.project_version())
ent_conf.set('PACKAGE_API_VERSION', gclib_api_version)
configure_file(input: 'gtkdocentities.ent.in', output: 'gtkdocentities.ent', configuration: ent_conf)

private_headers = [
  'config.h',

  'geocode-glib-private.h',
  'geocode-enum-types.h',
  'geocode-nominatim-test.h',
]

version_xml_conf = configuration_data()
version_xml_conf.set('VERSION', meson.project_version())
configure_file(input: 'version.xml.in',
               output: 'version.xml',
               configuration: version_xml_conf)

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(datadir, 'gtk-doc', 'html')

toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

gidocgen = find_program('gi-docgen', required: get_option('enable-gtk-doc'))

gidocgen_common_args = [
  '--quiet',
  '--no-namespace-dir',
]

if get_option('werror')
  gidocgen_common_args += ['--fatal-warnings']
endif

docs_dir = datadir / 'doc'
source_toml = configure_file(
  input: 'GeocodeGlib.toml.in',
  output: 'GeocodeGlib.toml',
  configuration: toml_conf,
  install: true,
  install_dir: docs_dir / 'geocode-glib-1',
)

custom_target('geocodeglib-doc',
  input: [ source_toml, geocode_gir[0] ],
  output: 'geocode-glib-1',
  command: [
    gidocgen,
    'generate',
    gidocgen_common_args,
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)
