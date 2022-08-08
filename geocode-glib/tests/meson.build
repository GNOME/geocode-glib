install_dir = get_option('prefix') / get_option('datadir') / 'installed-tests' / library_name
install_bindir = get_option('prefix') / get_option('libexecdir') / library_name

e = executable('geo-uri',
               'geo-uri.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_bindir)
test('Geo URI', e)
tests = ['geo-uri']

e = executable('geocode-glib',
               'geocode-nominatim-test.h',
               'geocode-nominatim-test.c',
               'geocode-glib.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_bindir)
env = ['G_TEST_SRCDIR=' + meson.current_source_dir()]
test('API test', e, env: env)
tests += ['geocode-glib']

e = executable('mock-backend',
               'mock-backend.c',
               dependencies: geocode_glib_dep,
               install: get_option('enable-installed-tests'),
               install_dir: install_bindir)
test('Test mock backend', e)
tests += ['mock-backend']

if get_option('enable-installed-tests')
  foreach test_name: tests
    conf_data = configuration_data()
    conf_data.set('PATH', install_dir)
    conf_data.set('BINPATH', install_bindir)
    conf_data.set('NAME', test_name)
    test_file = configure_file(
      configuration: conf_data,
      input: 'test-template.test.in',
      output: test_name + '.test',
    )

    install_data(test_file,
                 install_dir: install_dir)
  endforeach
  install_data('locale_format.json',
               'locale_name.json',
               'locale_name2.json',
               'nominatim-area.json',
               'nominatim-data-type-change.json',
               'nominatim-no-results.json',
               'nominatim-place_rank.json',
               'nominatim-rio.json',
               'osm_type0.json',
               'osm_type1.json',
               'osm_type2.json',
               'pi.json',
               'pub.json',
               'rev.json',
               'rev_fail.json',
               'search.json',
               'search_lat_long.json',
               'xep.json',
               install_dir: install_dir)
endif
