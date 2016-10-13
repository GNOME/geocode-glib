/*
   Copyright (C) 2011-2012 Bastien Nocera

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

#include <config.h>

#include <string.h>
#include <locale.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <geocode-glib/geocode-glib.h>
#include <geocode-glib/geocode-error.h>
#include <geocode-glib/geocode-reverse.h>
#include <geocode-glib/geocode-backend.h>
#include <geocode-glib/geocode-nominatim.h>
#include <geocode-glib/geocode-glib-private.h>

/**
 * SECTION:geocode-reverse
 * @short_description: Geocode reverse geocoding object
 * @include: geocode-glib/geocode-glib.h
 *
 * Contains functions for reverse geocoding using the
 * <ulink url="http://wiki.openstreetmap.org/wiki/Nominatim">OSM Nominatim APIs</ulink>
 **/

struct _GeocodeReversePrivate {
	GeocodeLocation *location;
	GeocodeBackend  *backend;
};

G_DEFINE_TYPE (GeocodeReverse, geocode_reverse, G_TYPE_OBJECT)

static void
geocode_reverse_finalize (GObject *gobject)
{
	GeocodeReverse *object = (GeocodeReverse *) gobject;

	g_clear_object (&object->priv->location);
	g_clear_object (&object->priv->backend);

	G_OBJECT_CLASS (geocode_reverse_parent_class)->finalize (gobject);
}

static void
geocode_reverse_class_init (GeocodeReverseClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	bindtextdomain (GETTEXT_PACKAGE, GEOCODE_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	gobject_class->finalize = geocode_reverse_finalize;

	g_type_class_add_private (klass, sizeof (GeocodeReversePrivate));
}

static void
geocode_reverse_init (GeocodeReverse *object)
{
	object->priv = G_TYPE_INSTANCE_GET_PRIVATE ((object), GEOCODE_TYPE_REVERSE, GeocodeReversePrivate);
}

/**
 * geocode_reverse_new_for_location:
 * @location: a #GeocodeLocation object
 *
 * Creates a new #GeocodeReverse to perform reverse geocoding with.
 * Use geocode_reverse_resolve_async() to perform the resolution.
 *
 * Returns: a new #GeocodeReverse. Use g_object_unref() when done.
 **/
GeocodeReverse *
geocode_reverse_new_for_location (GeocodeLocation *location)
{
	GeocodeReverse *object;

	g_return_val_if_fail (GEOCODE_IS_LOCATION (location), NULL);

	object = g_object_new (GEOCODE_TYPE_REVERSE, NULL);
	object->priv->location = g_object_ref (location);

	return object;
}

static void
ensure_backend (GeocodeReverse *object)
{
	/* If no backend is specified, default to the GNOME Nominatim backend */
	if (object->priv->backend == NULL)
		object->priv->backend = GEOCODE_BACKEND (geocode_nominatim_get_gnome ());
}

static void
backend_reverse_resolve_ready (GeocodeBackend *backend,
                               GAsyncResult   *res,
                               GTask          *task)
{
	GeocodePlace *place;
	GError *error = NULL;

	place = geocode_backend_reverse_resolve_finish (backend, res, &error);
	if (place)
		g_task_return_pointer (task, place, g_object_unref);
	else
		g_task_return_error (task, error);
	g_object_unref (task);
}

/**
 * geocode_reverse_resolve_async:
 * @object: a #GeocodeReverse representing a query
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously gets the result of a reverse geocoding
 * query using a web service. Use geocode_reverse_resolve() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_reverse_resolve_finish() to get the result of the operation.
 **/
void
geocode_reverse_resolve_async (GeocodeReverse     *object,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
	GTask *task;

	g_return_if_fail (GEOCODE_IS_REVERSE (object));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	ensure_backend (object);
	g_assert (object->priv->backend != NULL);

	task = g_task_new (object, cancellable, callback, user_data);
	geocode_backend_reverse_resolve_async (object->priv->backend,
	                                       object->priv->location,
	                                       cancellable,
	                                       (GAsyncReadyCallback) backend_reverse_resolve_ready,
	                                       g_object_ref (task));
	g_object_unref (task);
}

/**
 * geocode_reverse_resolve_finish:
 * @object: a #GeocodeReverse representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a reverse geocoding operation. See geocode_reverse_resolve_async().
 *
 * Returns: (transfer full): A #GeocodePlace instance, or %NULL in case of
 * errors. Free the returned instance with #g_object_unref() when done.
 **/
GeocodePlace *
geocode_reverse_resolve_finish (GeocodeReverse *object,
                                GAsyncResult   *res,
                                GError        **error)
{
	g_return_val_if_fail (GEOCODE_IS_REVERSE (object), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GEOCODE_PLACE (g_task_propagate_pointer (G_TASK (res), error));
}

/**
 * geocode_reverse_resolve:
 * @object: a #GeocodeReverse representing a query
 * @error: a #GError
 *
 * Gets the result of a reverse geocoding
 * query using a web service.
 *
 * Returns: (transfer full): A #GeocodePlace instance, or %NULL in case of
 * errors. Free the returned instance with #g_object_unref() when done.
 **/
GeocodePlace *
geocode_reverse_resolve (GeocodeReverse *object,
                         GError        **error)
{
	g_return_val_if_fail (GEOCODE_IS_REVERSE (object), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	ensure_backend (object);
	g_assert (object->priv->backend != NULL);

	return geocode_backend_reverse_resolve (object->priv->backend,
	                                        object->priv->location,
	                                        NULL,
	                                        error);
}

/**
 * geocode_reverse_set_backend:
 * @object: a #GeocodeReverse representing a query
 * @backend: (nullable): a #GeocodeBackend, or %NULL to use the default one.
 *
 * Specifies the backend to use in the reverse geocoding operation.
 *
 * If none is given, the default GNOME Nominatim server is used.
 *
 * Since: UNRELEASED
 */
void
geocode_reverse_set_backend (GeocodeReverse *object,
                             GeocodeBackend *backend)
{
	g_return_if_fail (GEOCODE_IS_REVERSE (object));
	g_return_if_fail (backend == NULL || GEOCODE_IS_BACKEND (backend));

	g_set_object (&object->priv->backend, backend);
}
