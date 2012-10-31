/*
   Copyright (C) 2012 Bastien Nocera

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

#ifndef GEOCODE_LOCATION_H
#define GEOCODE_LOCATION_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct {
	gdouble longitude;
	gdouble latitude;
	gint64  timestamp;
	char   *description;
} GeocodeLocation;

#define GEOCODE_TYPE_LOCATION (geocode_location_get_type ())

GType geocode_location_get_type (void) G_GNUC_CONST;

GeocodeLocation *geocode_location_new (gdouble latitude,
				       gdouble longitude);

GeocodeLocation *geocode_location_new_with_description (gdouble     latitude,
							gdouble     longitude,
							const char *description);

G_END_DECLS

#endif /* GEOCODE_LOCATION_H */
