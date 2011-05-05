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

typedef struct GeocodeObject        GeocodeObject;
typedef struct GeocodeObjectClass   GeocodeObjectClass;
typedef struct GeocodeObjectPrivate GeocodeObjectPrivate;

struct GeocodeObject {
	GObject parent_instance;
	GeocodeObjectPrivate *priv;
};

struct GeocodeObjectClass {
	GObjectClass parent_class;
};

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
