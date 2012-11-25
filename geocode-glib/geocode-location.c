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

#include <geocode-location.h>
#include <math.h>

#define EARTH_RADIUS_KM 6372.795

/**
 * SECTION:geocode-location
 * @short_description: Geocode location object
 * @include: geocode-glib/geocode-glib.h
 *
 * An object representing a location, with optional description.
 **/
static gpointer
geocode_location_copy (gpointer boxed)
{
	GeocodeLocation *from, *to;

	from = (GeocodeLocation *) boxed;
	to = g_new (GeocodeLocation, 1);
	to->longitude = from->longitude;
	to->latitude = from->latitude;
	to->timestamp = from->timestamp;
	to->description = from->description;

	return to;
}

/**
 * geocode_location_free:
 * @loc: a #GeocodeLocation object
 *
 * Frees a #GeocodeLocation object.
 **/
void
geocode_location_free (GeocodeLocation *loc)
{
	g_free (loc->description);
	g_free (loc);
}

G_DEFINE_BOXED_TYPE(GeocodeLocation, geocode_location, geocode_location_copy, geocode_location_free)

/**
 * geocode_location_new:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 *
 * Creates a new #GeocodeLocation object.
 *
 * Returns: a new #GeocodeLocation object. Use geocode_location_free() when done.
 **/
GeocodeLocation *
geocode_location_new (gdouble latitude,
		      gdouble longitude)
{
	GeocodeLocation *ret;
	GTimeVal tv;

	if (longitude < -180.0 || longitude > 180.0) {
		g_warning ("Invalid longitude %lf passed, using 0.0 instead", longitude);
		longitude = 0.0;
	}
	if (latitude < -90.0 || latitude > 90.0) {
		g_warning ("Invalid latitude %lf passed, using 0.0 instead", latitude);
		latitude = 0.0;
	}

	ret = g_new0 (GeocodeLocation, 1);
	ret->longitude = longitude;
	ret->latitude = latitude;
	g_get_current_time (&tv);
	ret->timestamp = tv.tv_sec;

	return ret;
}

/**
 * geocode_location_new_with_description:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 * @description: a description for the location
 *
 * Creates a new #GeocodeLocation object.
 *
 * Returns: a new #GeocodeLocation object. Use geocode_location_free() when done.
 **/
GeocodeLocation *
geocode_location_new_with_description (gdouble     latitude,
				       gdouble     longitude,
				       const char *description)
{
	GeocodeLocation *ret;

	ret = geocode_location_new (latitude, longitude);
	ret->description = g_strdup (description);

	return ret;
}

/**
 * geocode_location_get_distance_from:
 * @loca: a #GeocodeLocation
 * @locb: a #GeocodeLocation
 *
 * Calculates the distance in km, along the curvature of the Earth,
 * between 2 locations. Note that altitude changes are not
 * taken into account.
 *
 * Returns: a distance in km.
 **/
double
geocode_location_get_distance_from (GeocodeLocation *loca,
				    GeocodeLocation *locb)
{
	gdouble dlat, dlon, lat1, lat2;
	gdouble a, c;

	g_return_val_if_fail (loca != NULL || locb != NULL, 0.0);

	/* Algorithm from:
	 * http://www.movable-type.co.uk/scripts/latlong.html */

	dlat = (locb->latitude - loca->latitude) * M_PI / 180.0;
	dlon = (locb->longitude - loca->longitude) * M_PI / 180.0;
	lat1 = loca->latitude * M_PI / 180.0;
	lat2 = locb->latitude * M_PI / 180.0;

	a = sin (dlat / 2) * sin (dlat / 2) +
		sin (dlon / 2) * sin (dlon / 2) * cos (lat1) * cos (lat2);
	c = 2 * atan2 (sqrt (a), sqrt (1-a));
	return EARTH_RADIUS_KM * c;
}
