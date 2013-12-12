include $(top_srcdir)/Makefile.decl

EXTRA_DIST = geocode-glib.symbols

BUILT_GIRSOURCES =

lib_LTLIBRARIES = libgeocode-glib.la

libgeocode_glib_la_PUBLICSOURCES =			\
	geocode-location.c				\
	geocode-forward.c				\
	geocode-reverse.c				\
	geocode-glib.c					\
	geocode-error.c                                 \
	geocode-enum-types.c				\
	geocode-place.c					\
	geocode-bounding-box.c

libgeocode_glib_la_SOURCES =				\
	$(libgeocode_glib_la_PUBLICSOURCES)		\
	geocode-glib-private.h

libgeocode_glib_la_LIBADD = $(GEOCODE_LIBS) $(LIBS)

libgeocode_glib_la_LDFLAGS =				\
	-version-info $(GCLIB_LT_VERSION)		\
	-no-undefined					\
	$(AM_LDFLAGS)					\
	-export-symbols $(srcdir)/geocode-glib.symbols

GCGLIB_HEADER_FILES = \
	geocode-glib.h					\
	geocode-location.h				\
	geocode-forward.h				\
	geocode-reverse.h				\
	geocode-error.h                                 \
	geocode-place.h					\
	geocode-bounding-box.h

gcglibdir = $(includedir)/$(PACKAGE)-$(GCLIB_API_VERSION)/$(PACKAGE)
gcglib_HEADERS =					\
	$(GCGLIB_HEADER_FILES)				\
	geocode-enum-types.h

AM_CPPFLAGS = 						\
	-I$(top_srcdir)					\
	-I$(top_builddir) 				\
	-DTEST_SRCDIR=\""$(srcdir)/data/"\" 		\
	-DGEOCODE_LOCALEDIR=\"$(localedir)\"

AM_CFLAGS = $(GEOCODE_CFLAGS) $(COMMON_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED)

BUILT_SOURCES = \
               geocode-enum-types.c \
               geocode-enum-types.h

geocode-enum-types.h: $(GCGLIB_HEADER_FILES)
	$(AM_V_GEN) ($(GLIB_MKENUMS) \
		--fhead "#ifndef __GEOCODE_ENUM_TYPES_H__\n#define __GEOCODE_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GEOCODE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n\n/**\n * SECTION:geocode-enum-types\n * @short_description: Geocode enumerated types\n * @include: geocode-glib/geocode-glib.h\n *\n * The enumerated types defined and used by geocode-glib.\n **/" \
		--ftail "G_END_DECLS\n\n#endif /* __GEOCODE_ENUM_TYPES_H__ */" $^ > xgen-$(@F) \
		&& mv -f xgen-$(@F) $@)

geocode-enum-types.c: $(GCGLIB_HEADER_FILES) geocode-enum-types.h
	$(AM_V_GEN) ($(GLIB_MKENUMS) \
		--fhead "#include <geocode-glib/geocode-glib.h>\n#include <geocode-glib/geocode-enum-types.h>" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" $^ > xgen-$(@F) \
		&& mv -f xgen-$(@F) $@)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(top_srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)
CLEANFILES =

if HAVE_INTROSPECTION
introspection_files = $(libgeocode_glib_la_PUBLICSOURCES) $(gcglib_HEADERS)

GeocodeGlib-1.0.gir: libgeocode-glib.la
GeocodeGlib_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0 Json-1.0 Soup-2.4
GeocodeGlib_1_0_gir_PACKAGES = gobject-2.0 gmodule-2.0 glib-2.0 gio-2.0 libsoup-2.4 json-glib-1.0
GeocodeGlib_1_0_gir_CFLAGS = -I$(top_srcdir) -I$(top_builddir)
GeocodeGlib_1_0_gir_LIBS = libgeocode-glib.la
GeocodeGlib_1_0_gir_SCANNERFLAGS = \
	--symbol-prefix=geocode \
	--identifier-prefix=Geocode \
	--pkg-export=geocode-glib-1.0 \
	--c-include="geocode-glib/geocode-glib.h"
GeocodeGlib_1_0_gir_FILES = $(introspection_files)
INTROSPECTION_GIRS += GeocodeGlib-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

endif # HAVE_INTROSPECTION

TEST_PROGS += test-geouri test-gcglib
noinst_PROGRAMS = $(TEST_PROGS)

test_geouri_LDADD = libgeocode-glib.la $(GEOCODE_LIBS)
test_gcglib_LDADD = libgeocode-glib.la $(GEOCODE_LIBS)

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST +=					 \
	data/nominatim-rio.json			 \
	data/nominatim-area.json		 \
	data/nominatim-no-results.json

-include $(top_srcdir)/git.mk
