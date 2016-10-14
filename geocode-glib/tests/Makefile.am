TESTS = geo-uri geocode-glib
noinst_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = \
	G_TEST_SRCDIR="$(abs_srcdir)" \
	G_TEST_BUILDDIR="$(abs_builddir)" \
	G_DEBUG=gc-friendly \
	MALLOC_CHECK_=2 \
	MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256))

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DTEST_SRCDIR=\""$(srcdir)/../data/"\" \
	$(NULL)

AM_CFLAGS = \
	$(GEOCODE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

AM_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(NULL)

geocode_glib_SOURCES = \
	geocode-nominatim-test.h \
	geocode-nominatim-test.c \
	geocode-glib.c

geo_uri_LDADD = $(top_builddir)/geocode-glib/libgeocode-glib.la $(GEOCODE_LIBS)
geocode_glib_LDADD = $(top_builddir)/geocode-glib/libgeocode-glib.la $(GEOCODE_LIBS)

EXTRA_DIST = \
	locale_format.json \
	locale_name.json \
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
