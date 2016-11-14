include $(top_srcdir)/glib-tap.mk

installed_testdir = $(libexecdir)/installed-tests/geocode-glib-@GCLIB_API_VERSION@
installed_test_metadir = $(datadir)/installed-tests/geocode-glib-@GCLIB_API_VERSION@

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(NULL)

AM_CFLAGS = \
	$(GEOCODE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(NULL)

LDADD = \
	$(top_builddir)/geocode-glib/libgeocode-glib.la \
	$(GEOCODE_LIBS) \
	$(NULL)

test_programs = \
	geo-uri \
	geocode-glib \
	mock-backend \
	$(NULL)

geo_uri_SOURCES = geo-uri.c
geocode_glib_SOURCES = \
	geocode-nominatim-test.h \
	geocode-nominatim-test.c \
	geocode-glib.c
mock_backend_SOURCES = mock-backend.c

dist_test_data = \
	locale_format.json \
	locale_name.json \
	nominatim-area.json \
	nominatim-no-results.json \
	nominatim-rio.json \
	osm_type0.json \
	osm_type1.json \
	osm_type2.json \
	pub.json \
	rev.json \
	rev_fail.json \
	search.json \
	search_lat_long.json \
	xep.json \
	$(NULL)

-include $(top_srcdir)/git.mk
