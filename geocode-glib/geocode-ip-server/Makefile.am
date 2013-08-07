include $(top_srcdir)/Makefile.decl

BUILT_GIRSOURCES =

AM_CFLAGS = \
	-I$(top_srcdir) \
	$(GEOCODE_SERVER_CFLAGS) \
	$(COMMON_CFLAGS) \
	$(WARN_CFLAGS)  \
	$(DISABLE_DEPRECATED) \
	-DGEOIP_DATABASE_PATH=\""$(GEOIP_DATABASE_PATH)"\" \
	-DTEST_SRCDIR=\""$(top_srcdir)/geocode-glib/data/"\" \
	-DBUILDDIR=\""$(builddir)/"\"

bin_PROGRAMS = geoip-update

TEST_PROGS += test-geoipformat
noinst_PROGRAMS = geoip-lookup $(TEST_PROGS)

geoip_lookup_SOURCES = geoip-lookup.c geoip-server.h

geoip_lookup_LDADD = $(GEOCODE_SERVER_LIBS)
geoip_update_LDADD = $(GEOCODE_DBUPDATE_LIBS)
test_geoipformat_LDADD = $(GEOCODE_LIBS)

MAINTAINERCLEANFILES = Makefile.in


-include $(top_srcdir)/git.mk
