/*
   Copyright (C) 2011 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Authors: Bastien Nocera <hadess@hadess.net>

 */

#ifndef GEOCODE_ERROR_H
#define GEOCODE_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GEOCODE_ERROR_PARSE,
	GEOCODE_ERROR_NOT_SUPPORTED
} GeocodeError;

#define GEOCODE_ERROR (geocode_error_quark ())

GQuark geocode_error_quark (void);

G_END_DECLS

#endif /* GEOCODE_ERROR_H */
