include $(top_srcdir)/Makefile.decl

EXTRA_DIST = geocode-glib.symbols
BUILT_GIRSOURCES =

lib_LTLIBRARIES = libgeocode-glib.la

public_files =						\
	geocode-glib.c					\
	geocode-glib.h					\
	geocode-error.c					\
	geocode-error.h

libgeocode_glib_la_SOURCES =				\
	$(public_files)					\
	geocode-glib-private.h

libgeocode_glib_la_LIBADD = $(GEOCODE_LIBS)

libgeocode_glib_la_LDFLAGS =				\
	-version-info $(GCLIB_LT_VERSION)		\
	-no-undefined					\
	$(AM_LDFLAGS)
# FIXME
#	-export-symbols $(srcdir)/geocode-glib.symbols

gcglibdir = $(pkgincludedir)
gcglib_HEADERS = geocode-glib.h

AM_CFLAGS = -I$(srcdir) $(GEOCODE_CFLAGS) $(COMMON_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)
CLEANFILES =

if HAVE_INTROSPECTION
introspection_files = $(public_files)

GeocodeGlib-1.0.gir: libgeocode-glib.la
GeocodeGlib_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0 Json-1.0 SoupGNOME-2.4
GeocodeGlib_1_0_gir_PACKAGES = gobject-2.0 gmodule-2.0 glib-2.0 gio-2.0 libsoup-gnome-2.4 json-glib-1.0
GeocodeGlib_1_0_gir_CFLAGS = -I$(srcdir)
GeocodeGlib_1_0_gir_LIBS = libgeocode-glib.la
GeocodeGlib_1_0_gir_SCANNERFLAGS = --symbol-prefix=geocode_ --identifier-prefix=Geocode --pkg-export=geocode-glib-1.0
GeocodeGlib_1_0_gir_FILES = $(introspection_files)
INTROSPECTION_GIRS += GeocodeGlib-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

endif # HAVE_INTROSPECTION

TEST_PROGS += test-gcglib
noinst_PROGRAMS = $(TEST_PROGS)

test_gcglib_LDADD = libgeocode-glib.la $(GEOCODE_LIBS)

MAINTAINERCLEANFILES = Makefile.in

