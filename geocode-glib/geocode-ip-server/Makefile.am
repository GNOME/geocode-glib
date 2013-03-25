include $(top_srcdir)/Makefile.decl

BUILT_GIRSOURCES =

AM_CFLAGS = -I$(top_srcdir) $(GEOCODE_SERVER_CFLAGS) $(COMMON_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED) -DGEOIP_DATABASE_PATH=\""$(GEOIP_DATABASE_PATH)"\"

bin_PROGRAMS = geoip-update

TEST_PROGS += geoip-lookup
noinst_PROGRAMS = $(TEST_PROGS)

geoip_lookup_SOURCES = geoip-lookup.c geoip-server.h

geoip_lookup_LDADD = $(GEOCODE_SERVER_LIBS)
geoip_update_LDADD = $(GEOCODE_DBUPDATE_LIBS)

MAINTAINERCLEANFILES = Makefile.in

