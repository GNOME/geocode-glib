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

typedef struct _GeocodeLocation GeocodeLocation;

/**
 * GeocodeLocation:
 * @longitude: a longitude, in degrees
 * @latitude: a latitude, in degrees
 * @accuracy: accuracy of location, in meters
 * @timestamp: a timestamp in seconds since <ulink url="http://en.wikipedia.org/wiki/Unix_epoch">Epoch</ulink>.
 * @description: a description for display
 *
 * The #GeocodeLocation structure represents a location
 * on earth, with an optional description.
 **/
struct _GeocodeLocation {
	gdouble longitude;
	gdouble latitude;
	gdouble accuracy;
	gint64  timestamp;
	char   *description;
};

#define GEOCODE_LOCATION_ACCURACY_UNKNOWN   -1
#define GEOCODE_LOCATION_ACCURACY_STREET    1000    /*    1 km */
#define GEOCODE_LOCATION_ACCURACY_CITY      25000   /*   25 km */
#define GEOCODE_LOCATION_ACCURACY_REGION    50000   /*   50 km */
#define GEOCODE_LOCATION_ACCURACY_COUNTRY   150000  /*  150 km */
#define GEOCODE_LOCATION_ACCURACY_CONTINENT 3000000 /* 3000 km */

#define GEOCODE_TYPE_LOCATION (geocode_location_get_type ())

GType geocode_location_get_type (void) G_GNUC_CONST;

GeocodeLocation *geocode_location_new (gdouble latitude,
                                       gdouble longitude,
                                       gdouble accuracy);

GeocodeLocation *geocode_location_new_with_description (gdouble     latitude,
                                                        gdouble     longitude,
                                                        gdouble     accuracy,
                                                        const char *description);

double geocode_location_get_distance_from (GeocodeLocation *loca,
					   GeocodeLocation *locb);

void geocode_location_set_description (GeocodeLocation *loc,
				       const char      *description);

void geocode_location_free (GeocodeLocation *loc);

G_END_DECLS

#endif /* GEOCODE_LOCATION_H */
