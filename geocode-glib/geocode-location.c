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

static void
geocode_location_free (gpointer boxed)
{
	GeocodeLocation *loc;

	loc = (GeocodeLocation *) boxed;
	g_free (loc->description);
	g_free (loc);
}

G_DEFINE_BOXED_TYPE(GeocodeLocation, geocode_location, geocode_location_copy, geocode_location_free)

GeocodeLocation *
geocode_location_new (gdouble latitude,
		      gdouble longitude)
{
	GeocodeLocation *ret;
	GTimeVal tv;

	ret = g_new0 (GeocodeLocation, 1);
	ret->longitude = longitude;
	ret->latitude = latitude;
	g_get_current_time (&tv);
	ret->timestamp = tv.tv_sec;

	return ret;
}

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
