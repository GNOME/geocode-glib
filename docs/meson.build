if get_option('enable-gtk-doc') and not enable_gir
  error('API reference requires introspection.')
endif

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
