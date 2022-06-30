install_dir = get_option('libexecdir') + '/installed-tests/' + library_name

e = executable('geo-uri',
               'geo-uri.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_dir)
test('Geo URI', e)

e = executable('geocode-glib',
               'geocode-nominatim-test.h',
               'geocode-nominatim-test.c',
               'geocode-glib.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_dir)
env = ['G_TEST_SRCDIR=' + meson.current_source_dir()]
test('API test', e, env: env)

e = executable('mock-backend',
               'mock-backend.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_dir)
test('Test mock backend', e)

if get_option('enable-installed-tests')
  install_data('locale_format.json',
               'locale_name.json',
               'nominatim-area.json',
               'nominatim-data-type-change.json',
               'nominatim-no-results.json',
               'nominatim-place_rank.json',
               'nominatim-rio.json',
               'osm_type0.json',
               'osm_type1.json',
               'osm_type2.json',
               'pub.json',
               'rev.json',
               'rev_fail.json',
               'search.json',
               'search_lat_long.json',
               'xep.json',
               install_dir: install_dir)
endif
