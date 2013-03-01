AC_PREREQ([2.63])

m4_define(gclib_version_major, 0)
m4_define(gclib_version_minor, 99)
m4_define(gclib_version_micro, 1)

AC_INIT([geocode-glib],
        [gclib_version_major.gclib_version_minor.gclib_version_micro],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=geocode-glib],
        [geocode-glib])

AC_CONFIG_SRCDIR([geocode-glib/test-gcglib.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 dist-xz no-dist-gzip check-news])

# Before making a release, the GCLIB_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
GCLIB_LT_VERSION=0:0:0
AC_SUBST(GCLIB_LT_VERSION)

AM_MAINTAINER_MODE([enable])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_LANG([C])

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# i18n support
IT_PROG_INTLTOOL([0.41.0])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=geocode-glib
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
					[The name of the gettext domain])

dnl gtk-doc checks
GTK_DOC_CHECK([1.13],[--flavour no-tmpl])

dnl Requires for the library
PKG_CHECK_MODULES(GEOCODE,
		  gio-2.0 >= 2.34
		  json-glib-1.0 >= 0.14
		  libsoup-2.4)

# Requires for the server
PKG_CHECK_MODULES(GEOCODE_SERVER,
		  gio-2.0 >= 2.34
		  json-glib-1.0 >= 0.14
		  libsoup-2.4
                  geoip)

# Requires for the database update
PKG_CHECK_MODULES(GEOCODE_DBUPDATE,
		  gio-2.0 >= 2.34)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GEOIP_DATABASE_PATH=`$PKG_CONFIG --variable=databasedir geoip`
AC_SUBST(GEOIP_DATABASE_PATH)

GOBJECT_INTROSPECTION_CHECK([0.6.3])

GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS
GNOME_MAINTAINER_MODE_DEFINES

if test "$GCC" = "yes" ; then
	CFLAGS="\
	-Wall \
	-Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs -Wpointer-arith \
	-Wcast-align -Wsign-compare -Wredundant-decls \
	-fno-strict-aliasing \
	$CFLAGS"
fi

AC_CONFIG_FILES([
          Makefile
	  geocode-glib.pc
	  docs/Makefile
	  docs/version.xml
	  geocode-glib/Makefile
	  po/Makefile.in
	  geocode-glib/geocode-ip-server/Makefile
])
AC_OUTPUT

