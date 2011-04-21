AC_PREREQ(2.52)

m4_define(diglib_version_major, 0)
m4_define(diglib_version_minor, 9)
m4_define(diglib_version_micro, 9)

AC_INIT([geocode-glib],
        [diglib_version_major.diglib_version_minor.diglib_version_micro],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=geocode-glib])

AC_CONFIG_SRCDIR([geocode-glib/test-gcglib.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 no-dist-gzip check-news])

# Before making a release, the GCLIB_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
GCLIB_LT_VERSION=0:0:0
AC_SUBST(GCLIB_LT_VERSION)

AM_MAINTAINER_MODE

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_LANG_C
AC_PROG_CC
AC_PROG_INSTALL

dnl Initialize libtool
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

GETTEXT_PACKAGE=geocode-glib
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
					[The name of the gettext domain])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

dnl gtk-doc checks
GTK_DOC_CHECK(1.9)

AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/share/locale", [Directory for the localization files])

dnl Requires for the library
PKG_CHECK_MODULES(GEOCODE,
		  gio-2.0
		  json-glib-1.0 >= 0.13.1
		  libsoup-2.4)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GOBJECT_INTROSPECTION_CHECK([0.6.3])

GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS
GNOME_MAINTAINER_MODE_DEFINES

GNOME_DOC_INIT

AC_OUTPUT(Makefile
	  geocode-glib.pc
	  docs/Makefile
	  docs/version.xml
	  geocode-glib/Makefile
	  po/Makefile.in
)

