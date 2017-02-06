AC_PREREQ([2.63])

m4_define(gclib_version_major, 3)
m4_define(gclib_version_minor, 23)
m4_define(gclib_version_micro, 90)

AC_INIT([geocode-glib],
        [gclib_version_major.gclib_version_minor.gclib_version_micro],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=geocode-glib],
        [geocode-glib])
AX_IS_RELEASE([git-directory])

AC_CONFIG_SRCDIR([geocode-glib/geocode-glib.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_REQUIRE_AUX_FILE([tap-driver.sh])

AM_INIT_AUTOMAKE([1.11 dist-xz no-dist-gzip check-news subdir-objects])
AX_CHECK_ENABLE_DEBUG([yes])

# Before making a release, the GCLIB_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
GCLIB_LT_VERSION=0:0:0
AC_SUBST(GCLIB_LT_VERSION)

GCLIB_API_VERSION=1.0
AC_SUBST(GCLIB_API_VERSION)

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
# FIXME: Remove AM_GNU_GETTEXT_VERSION once autoreconf supports REQUIRE_VERSION
AM_GNU_GETTEXT_VERSION([0.19.6])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.6])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=geocode-glib
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
					[The name of the gettext domain])

dnl gtk-doc checks
GTK_DOC_CHECK([1.13],[--flavour no-tmpl])

AC_CHECK_LIB([m],[atan2])

dnl Requires for the library
PKG_CHECK_MODULES(GEOCODE,
		  gio-2.0 >= 2.44
		  json-glib-1.0 >= 0.99.2
		  libsoup-2.4 >= 2.42)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GOBJECT_INTROSPECTION_CHECK([0.6.3])

AX_COMPILER_FLAGS([WARN_CFLAGS],[WARN_LDFLAGS])

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

# installed-tests
AC_ARG_ENABLE([always_build_tests],
              AS_HELP_STRING([--enable-always-build-tests],
                             [Enable always building tests (default: yes)]),,
              [enable_always_build_tests=yes])
AC_ARG_ENABLE([installed_tests],
              AS_HELP_STRING([--enable-installed-tests],
                             [Install test programs (default: no)]),,
              [enable_installed_tests=no])

AM_CONDITIONAL([ENABLE_ALWAYS_BUILD_TESTS],
               [test "$enable_always_build_tests" = "yes"])
AC_SUBST([ENABLE_ALWAYS_BUILD_TESTS],[$enable_always_build_tests])

AM_CONDITIONAL([ENABLE_INSTALLED_TESTS],
               [test "$enable_installed_tests" = "yes"])
AC_SUBST([ENABLE_INSTALLED_TESTS],[$enable_installed_tests])

# Changelog generation
AX_GENERATE_CHANGELOG

AC_CONFIG_FILES([
          Makefile
	  geocode-glib-1.0.pc
	  docs/Makefile
	  docs/version.xml
	  geocode-glib/Makefile
	  geocode-glib/tests/Makefile
	  po/Makefile.in
	  icons/Makefile
])
AC_OUTPUT

