#!/bin/sh -x

GLIB2_VER=`rpm -q glib2`
GLIBC_VER=`rpm -q glibc`

# Undo delangification present in the Fedora Docker images
rm -f /etc/rpm/macros.image-language-conf

dnf install -y $@

# Only reinstall glibc and glib2 if they weren't updated
RPMS=""
if [ "$GLIB2_VER" == "`rpm -q glib2`" ] ; then
	RPMS="glib2"
fi
if [ "$GLIBC_VER" == "`rpm -q glibc`" ] ; then
	if [ -z "$RPMS" ] ; then
		RPMS="glibc"
	else
		RPMS="$RPMS glibc"
	fi
fi

if [ ! -z "$RPMS" ] ; then
	dnf reinstall -y $RPMS
fi
