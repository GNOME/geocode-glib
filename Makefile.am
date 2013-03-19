ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = geocode-glib po docs

EXTRA_DIST = intltool-extract.in intltool-update.in intltool-merge.in gtk-doc.make

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc=yes --enable-introspection=yes

DISTCLEANFILES = intltool-extract intltool-update intltool-merge

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp missing install-sh mkinstalldirs \
	intltool-extract.in intltool-update.in intltool-merge.in \
	geocode-glib-1.0.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geocode-glib-1.0.pc

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

