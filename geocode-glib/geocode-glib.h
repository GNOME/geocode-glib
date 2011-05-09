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

#ifndef GEOCODE_GLIB_H
#define GEOCODE_GLIB_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

GType geocode_object_get_type (void) G_GNUC_CONST;

#define GEOCODE_TYPE_OBJECT                  (geocode_object_get_type ())
#define GEOCODE_OBJECT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCODE_TYPE_OBJECT, GeocodeObject))
#define GEOCODE_IS_OBJECT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCODE_TYPE_OBJECT))
#define GEOCODE_OBJECT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCODE_TYPE_OBJECT, GeocodeObjectClass))
#define GEOCODE_IS_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCODE_TYPE_OBJECT))
#define GEOCODE_OBJECT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GEOCODE_TYPE_OBJECT, GeocodeObjectClass))

/**
 * GeocodeObject:
 *
 * All the fields in the #GeocodeObject structure are private and should never be accessed directly.
**/
typedef struct _GeocodeObject        GeocodeObject;
typedef struct _GeocodeObjectClass   GeocodeObjectClass;
typedef struct _GeocodeObjectPrivate GeocodeObjectPrivate;

struct _GeocodeObject {
	GObject parent_instance;
	GeocodeObjectPrivate *priv;
};

struct _GeocodeObjectClass {
	GObjectClass parent_class;
};

/* Known metadata fields */

/**
 * GEOCODE_OBJECT_FIELD_LONGITUDE:
 *
 * Metadata field for a result, representing the longitude
 * at the center of the matched area.
 **/
#define GEOCODE_OBJECT_FIELD_LONGITUDE		"longitude"

/**
 * GEOCODE_OBJECT_FIELD_LATITUDE:
 *
 * Metadata field for a result, representing the latitude
 * at the center of the matched area.
 **/
#define GEOCODE_OBJECT_FIELD_LATITUDE		"latitude"

/**
 * GEOCODE_OBJECT_FIELD_RADIUS:
 *
 * Metadata field for a result, representing the radius of the
 * matched area in meters.
 **/
#define GEOCODE_OBJECT_FIELD_RADIUS		"radius"

/**
 * GEOCODE_OBJECT_FIELD_TIMEZONE:
 *
 * Metadata field for a result, representing the timezone
 * of the matched area.
 **/
#define GEOCODE_OBJECT_FIELD_TIMEZONE		"timezone"

/**
 * GEOCODE_OBJECT_FIELD_CITY:
 *
 * Metadata field for a result, representing the city
 * of the matched area.
 **/
#define GEOCODE_OBJECT_FIELD_CITY		"city"

/**
 * GEOCODE_OBJECT_FIELD_COUNTRY:
 *
 * Metadata field for a result, representing the country
 * of the matched area.
 **/
#define GEOCODE_OBJECT_FIELD_COUNTRY		"country"

/**
 * GEOCODE_OBJECT_FIELD_AIRPORT:
 *
 * Metadata field for a result, representing the closest
 * airport to the matched area.
**/
#define GEOCODE_OBJECT_FIELD_AIRPORT		"airport"

/**
 * GEOCODE_OBJECT_FIELD_UZIP:
 *
 * Metadata field for a result, representing the unique
 * zip code (or postal code) for the matched area.
**/
#define GEOCODE_OBJECT_FIELD_UZIP		"uzip"

/**
 * GEOCODE_OBJECT_FIELD_COUNTRYCODE:
 *
 * Metadata field for a result, representing the country
 * code of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_COUNTRYCODE	"countrycode"

/**
 * GEOCODE_OBJECT_FIELD_NAME:
 *
 * Metadata field for a result, representing the name
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_NAME		"name"

/**
 * GEOCODE_OBJECT_FIELD_LINE1:
 *
 * Metadata field for a result, representing the line1
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_LINE1		"line1"

/**
 * GEOCODE_OBJECT_FIELD_LINE2:
 *
 * Metadata field for a result, representing the line2
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_LINE2		"line2"

/**
 * GEOCODE_OBJECT_FIELD_LINE3:
 *
 * Metadata field for a result, representing the line3
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_LINE3		"line3"

/**
 * GEOCODE_OBJECT_FIELD_LINE4:
 *
 * Metadata field for a result, representing the line4
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_LINE4		"line4"

/**
 * GEOCODE_OBJECT_FIELD_HOUSE:
 *
 * Metadata field for a result, representing the house
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_HOUSE		"house"

/**
 * GEOCODE_OBJECT_FIELD_STREET:
 *
 * Metadata field for a result, representing the street
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_STREET		"street"

/**
 * GEOCODE_OBJECT_FIELD_POSTAL:
 *
 * Metadata field for a result, representing the postal
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_POSTAL		"postal"

/**
 * GEOCODE_OBJECT_FIELD_NEIGHBORHOOD:
 *
 * Metadata field for a result, representing the neighborhood
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_NEIGHBORHOOD	"neighborhood"

/**
 * GEOCODE_OBJECT_FIELD_COUNTY:
 *
 * Metadata field for a result, representing the county
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_COUNTY		"county"

/**
 * GEOCODE_OBJECT_FIELD_HASH:
 *
 * Metadata field for a result, representing the hash
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_HASH		"hash"

/**
 * GEOCODE_OBJECT_FIELD_STATECODE:
 *
 * Metadata field for a result, representing the statecode
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_STATECODE		"statecode"

/**
 * GEOCODE_OBJECT_FIELD_STATE:
 *
 * Metadata field for a result, representing the state
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_STATE		"state"

/**
 * GEOCODE_OBJECT_FIELD_COUNTYCODE:
 *
 * Metadata field for a result, representing the county code
 * of the matched area.
**/
#define GEOCODE_OBJECT_FIELD_COUNTYCODE		"countycode"

/**
 * GEOCODE_OBJECT_FIELD_OFFSETLAT:
 *
 * Metadata field for a result, representing the latitude of
 * the offset point in degrees, representing building/parcel
 * setback from road center-line for the matched area.
**/
#define GEOCODE_OBJECT_FIELD_OFFSETLAT		"offsetlat"

/**
 * GEOCODE_OBJECT_FIELD_OFFSETLON:
 *
 * Metadata field for a result, representing the longitude
 * the offset point in degrees, representing building/parcel
 * setback from road center-line for the matched area.
**/
#define GEOCODE_OBJECT_FIELD_OFFSETLON		"offsetlon"

GeocodeObject *geocode_object_new (void);
GeocodeObject *geocode_object_new_for_params (GHashTable *params);
GeocodeObject *geocode_object_new_for_coords (gdouble     latitude,
					      gdouble     longitude);

void geocode_object_add (GeocodeObject *object,
			 const char    *key,
			 const char    *value);

void  geocode_object_resolve_async  (GeocodeObject       *object,
				     GCancellable        *cancellable,
				     GAsyncReadyCallback  callback,
				     gpointer             user_data);

GHashTable *geocode_object_resolve_finish (GeocodeObject  *object,
					   GAsyncResult   *res,
					   GError        **error);

GHashTable * geocode_object_resolve (GeocodeObject  *object,
				     GError        **error);

gboolean geocode_object_get_coords (GHashTable *results,
				    gdouble    *longitude,
				    gdouble    *latitude);

G_END_DECLS

#endif /* GEOCODE_GLIB_H */
