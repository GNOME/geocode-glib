#!/bin/bash

# taken from http://git.gnome.org/browse/totem/tree/browser-plugin/tests/launch-web-server.sh with some necessary modifications

# Port to listen on
PORT=12345


usage()
{
    echo "Usage: ./`basename $0` <--remote> [stop | start]"
echo " --remote: allow for connections from remote machines"
exit 1
}


# Find Apache first

HTTPD=${APACHE_HTTPD:-/usr/sbin/apache2}

if [ -z $HTTPD ] ; then
    echo "Could not find httpd at the usual locations"
    exit 1
fi

# Check whether we in the right directory

if [ ! -f `basename $0` ] ; then
    echo "You need to launch `basename $0` from within its directory"
    echo "eg: ./`basename $0` <--remote> [stop | start]"
echo " --remote: allow for connections from remote machines"
exit 1
fi

ROOTDIR=`dirname $0`/root

# See if we shoud stop the web server

if [ -z $1 ] ; then
    usage $0
fi

if [ x$1 = x"--remote" ] ; then
    ADDRESS=
    shift
else
    ADDRESS=127.0.0.1
fi

if [ x$1 = xstop ] ; then
    echo "Trying to stop $HTTPD(`cat root/pid`)"
    pushd root/ > /dev/null
    $HTTPD -f `pwd`/conf -k stop
    popd > /dev/null
    exit
elif [ x$1 != xstart ] ; then
    usage $0
fi

# Setup the ServerRoot

if [ ! -d $ROOTDIR ] ; then
    mkdir -p root/ || ( echo "Could not create the ServerRoot" ; exit 1 )
fi

DOCDIR=`pwd`
CGI_BIN_DIR=`pwd`/cgi-bin/
server_exec=geoip-lookup

#set up the cgi-bin directory
if [ ! -d $CGI_BIN_DIR ] ; then
    mkdir -p cgi-bin/ || ( echo "Could not create the cgi-bin directory" ; exit 1 )
    cp ../$server_exec cgi-bin/ || ( echo "Could not copy the server executable to the cgi-bin directory" ; exit 1 )
fi

pushd root/ > /dev/null
# Resolve the relative ROOTDIR path
ROOTDIR=`pwd`
if [ -f pid ] && [ -f conf ] ; then
    $HTTPD -f $ROOTDIR/conf -k stop
    sleep 2
fi
rm -f conf pid lock log access_log

# Setup the config file

cat > conf <<EOF
LoadModule alias_module /usr/lib/apache2/modules/mod_alias.so
LoadModule auth_basic_module /usr/lib/apache2/modules/mod_auth_basic.so
LoadModule authn_file_module /usr/lib/apache2/modules/mod_authn_file.so
LoadModule authz_default_module /usr/lib/apache2/modules/mod_authz_default.so
LoadModule authz_groupfile_module /usr/lib/apache2/modules/mod_authz_groupfile.so
LoadModule authz_host_module /usr/lib/apache2/modules/mod_authz_host.so
LoadModule authz_user_module /usr/lib/apache2/modules/mod_authz_user.so
LoadModule autoindex_module /usr/lib/apache2/modules/mod_autoindex.so
LoadModule cgid_module /usr/lib/apache2/modules/mod_cgid.so
LoadModule deflate_module /usr/lib/apache2/modules/mod_deflate.so
LoadModule dir_module /usr/lib/apache2/modules/mod_dir.so
LoadModule env_module /usr/lib/apache2/modules/mod_env.so
LoadModule mime_module /usr/lib/apache2/modules/mod_mime.so
LoadModule negotiation_module /usr/lib/apache2/modules/mod_negotiation.so
LoadModule reqtimeout_module /usr/lib/apache2/modules/mod_reqtimeout.so
LoadModule setenvif_module /usr/lib/apache2/modules/mod_setenvif.so
LoadModule status_module /usr/lib/apache2/modules/mod_status.so


ServerName localhost
ServerRoot "$ROOTDIR"
PidFile pid
#LockFile lock
# LogLevel crit
LogLevel info
ErrorLog log
LogFormat "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"" combined
CustomLog access_log combined
TypesConfig /etc/mime.types
# Socket for cgid communication
ScriptSock cgisock

<VirtualHost *:$PORT>
  DocumentRoot "$DOCDIR"

  <Directory />
    Options FollowSymLinks
    AllowOverride None
  </Directory>

  <Directory "$DOCDIR">
   Options Indexes FollowSymLinks MultiViews
   AllowOverride None
   Order allow,deny
   allow from all
  </Directory>

  ScriptAlias /cgi-bin/ "$CGI_BIN_DIR"
  <Directory "$CGI_BIN_DIR">
   AllowOverride None
   Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch
   Order allow,deny
   Allow from all
  </Directory>

</VirtualHost>

StartServers 1
EOF

popd > /dev/null

# Launch!


#$HTTPD -f $ROOTDIR/conf -C "Listen 127.0.0.1:$PORT"
if [ -z $ADDRESS ] ; then
$HTTPD -f $ROOTDIR/conf -C "Listen $PORT"
else
$HTTPD -f $ROOTDIR/conf -C "Listen ${ADDRESS}:$PORT"
fi
echo "Please start debugging at http://localhost:$PORT/"