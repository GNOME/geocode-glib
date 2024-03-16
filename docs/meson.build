ent_conf = configuration_data()
ent_conf.set('PACKAGE', 'Geocode-Glib')
ent_conf.set('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/geocode-glib/-/issues/')
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

gnome.gtkdoc(library_name,
             main_xml: 'geocode-glib-docs.xml',
             src_dir: [
               join_paths(meson.source_root(), 'geocode-glib'),
               join_paths(meson.build_root(), 'geocode-glib'),
             ],
             dependencies: geocode_glib_dep,
             gobject_typesfile: 'geocode-glib.types',
             scan_args: [
               '--rebuild-types',
               '--ignore-decorators=GEOCODE_EXTERN',
               '--ignore-headers=' + ' '.join(private_headers),
             ],
             fixxref_args: [
               '--html-dir=@0@'.format(docpath),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
               '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
             ],
             install: true)

