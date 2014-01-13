ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = geocode-glib po docs icons

EXTRA_DIST = gtk-doc.make

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc=yes --enable-introspection=yes

DISTCLEANFILES = gtk-doc.make

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	build-aux/ \
	INSTALL \
	omf.make \
	gtk-doc.make \
	ABOUT-NLS \
	config.rpath \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	m4/gtk-doc.m4 \
	m4/intltool.m4 \
	geocode-glib-1.0.pc \
	$(NULL)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = geocode-glib-1.0.pc

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog


-include $(top_srcdir)/git.mk
