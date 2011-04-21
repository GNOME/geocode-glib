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

#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <geocode-glib.h>
#include <geocode-error.h>
#include <geocode-glib-private.h>

struct GeocodeObjectPrivate {
	GHashTable *ht;
	guint reverse : 1;
	guint flags_added : 1;
};

G_DEFINE_TYPE (GeocodeObject, geocode_object, G_TYPE_OBJECT)

static void
geocode_object_finalize (GObject *gobject)
{
	GeocodeObject *object = (GeocodeObject *) object;

	g_hash_table_destroy (object->priv->ht);
	object->priv->ht = NULL;

	G_OBJECT_CLASS (geocode_object_parent_class)->finalize (gobject);
}

static void
geocode_object_class_init (GeocodeObjectClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = geocode_object_finalize;

	g_type_class_add_private (klass, sizeof (GeocodeObjectPrivate));
}

static void
geocode_object_init (GeocodeObject *object)
{
	object->priv = G_TYPE_INSTANCE_GET_PRIVATE ((object), GEOCODE_TYPE_OBJECT, GeocodeObjectPrivate);
	object->priv->ht = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, g_free);
}

GeocodeObject *
geocode_object_new (void)
{
	return g_object_new (GEOCODE_TYPE_OBJECT, NULL);
}

GeocodeObject *
geocode_object_new_for_params (GHashTable *params)
{
	/* FIXME */
	return NULL;
}

GeocodeObject *
geocode_object_new_for_coords (gdouble     latitude,
			       gdouble     longitude)
{
	GeocodeObject *object;

	g_return_val_if_fail (longitude >= -180.0 && longitude <= 180.0, NULL);
	g_return_val_if_fail (latitude >= -90.0 && latitude <= 90.0, NULL);

	object = geocode_object_new ();
	object->priv->reverse = TRUE;

	g_hash_table_insert (object->priv->ht,
			     g_strdup ("location"),
			     g_strdup_printf ("%g, %g", latitude, longitude));

	return object;
}

void
geocode_object_add (GeocodeObject *object,
		    const char    *key,
		    const char    *value)
{
	g_return_if_fail (GEOCODE_IS_OBJECT (object));
	g_return_if_fail (key != NULL);

	g_hash_table_insert (object->priv->ht,
			     g_strdup (key),
			     g_strdup (value));
}

GHashTable *
_geocode_parse_json (const char *contents,
		     GError    **error)
{
	GHashTable *ret;
	JsonParser *parser;
	JsonNode *root;
	JsonReader *reader;
	gint64 err_code;
	guint i;
	const GError *err = NULL;
	char **members;

	ret = NULL;

	parser = json_parser_new ();
	if (json_parser_load_from_data (parser, contents, -1, error) == FALSE) {
		g_object_unref (parser);
		return ret;
	}

	root = json_parser_get_root (parser);
	reader = json_reader_new (root);

	if (json_reader_read_member (reader, "ResultSet") == FALSE)
		goto parse;

	if (json_reader_read_member (reader, "Error") == FALSE)
		goto parse;

	err_code = json_reader_get_int_value (reader);
	json_reader_end_member (reader);

	if (err_code != 0) {
		const char *msg;

		json_reader_read_member (reader, "ErrorMessage");
		msg = json_reader_get_string_value (reader);
		json_reader_end_member (reader);
		if (msg && *msg == '\0')
			msg = NULL;

		switch (err_code) {
		case 1:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "Query not supported");
			break;
		default:
			if (msg == NULL)
				g_set_error (error, GEOCODE_ERROR, GEOCODE_ERROR_PARSE, "Unknown error code %"G_GINT64_FORMAT, err_code);
			else
				g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_PARSE, msg);
			break;
		}
		g_object_unref (parser);
		g_object_unref (reader);
		return NULL;
	}

	if (json_reader_read_member (reader, "Results") == FALSE)
		goto parse;

	if (json_reader_read_element (reader, 0) == FALSE)
		goto parse;

	members = json_reader_list_members (reader);

	/* Yay, start adding data */
	ret = g_hash_table_new_full (g_str_hash, g_str_equal,
				     g_free, g_free);

	for (i = 0; members[i] != NULL; i++) {
		const char *value;

		json_reader_read_member (reader, members[i]);

		if (g_str_equal (members[i], "radius") ||
		    g_str_equal (members[i], "quality")) {
			gint64 num;

			num = json_reader_get_int_value (reader);
			g_hash_table_insert (ret, g_strdup (members[i]), g_strdup_printf ("%"G_GINT64_FORMAT, num));
			json_reader_end_member (reader);
			continue;
		}

		value = json_reader_get_string_value (reader);
		if (value && *value == '\0')
			value = NULL;

		if (value != NULL)
			g_hash_table_insert (ret, g_strdup (members[i]), g_strdup (value));
		json_reader_end_member (reader);
	}
	g_strfreev (members);

	return ret;

parse:
	if (ret != NULL)
		g_hash_table_destroy (ret);
	err = json_reader_get_error (reader);
	g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_PARSE, err->message);
	g_object_unref (parser);
	g_object_unref (reader);
	return NULL;
}

static void
on_query_data_loaded (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
	GFile *query;
	GError *error = NULL;
	char *contents;
	GHashTable *ret;

	query = G_FILE (source_object);
	if (g_file_load_contents_finish (query,
					 res,
					 &contents,
					 NULL,
					 NULL,
					 &error) == FALSE) {
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		g_error_free (error);
		return;
	}

	ret = _geocode_parse_json (contents, &error);
	g_free (contents);

	if (ret == NULL) {
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		g_error_free (error);
		return;
	}

	g_simple_async_result_set_op_res_gpointer (simple, ret, NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
}

static GFile *
get_query_for_params (GeocodeObject *object)
{
	GFile *ret;
	char *params, *uri;

	if (!object->priv->flags_added) {
		geocode_object_add (object, "appid", "zznSbDjV34HRU5CXQc4D3qE1DzCsJTaKvWTLhNJxbvI_JTp1hIncJ4xTSJFRgjE-");
		geocode_object_add (object, "flags", "QJT");
		object->priv->flags_added = TRUE;
	}
	if (object->priv->reverse)
		geocode_object_add (object, "gflags", "R");

	params = soup_form_encode_hash (object->priv->ht);

	uri = g_strdup_printf ("http://where.yahooapis.com/geocode?%s", params);
	g_free (params);

	ret = g_file_new_for_uri (uri);
	g_free (uri);

	return ret;
}

/**
 * geocode_object_resolve_async:
 * @object: a #GeocodeObject representing a query
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously gets the result of a geocoding or reverse geocoding
 * query using a web service.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_object_resolve_finish() to get the result of the operation.
 **/
void
geocode_object_resolve_async (GeocodeObject       *object,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	GSimpleAsyncResult *simple;
	GFile *query;

	g_return_if_fail (GEOCODE_IS_OBJECT (object));

	simple = g_simple_async_result_new (G_OBJECT (object),
					    callback,
					    user_data,
					    geocode_object_resolve_async);

	query = get_query_for_params (object);
	g_file_load_contents_async (query,
				    cancellable,
				    on_query_data_loaded,
				    simple);
	g_object_unref (query);
}

/**
 * geocode_object_resolve_finish:
 * @object: a #GeocodeObject representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a query operation. See geocode_object_resolve_async().
 *
 * Returns: a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GHashTable *
geocode_object_resolve_finish (GeocodeObject       *object,
			       GAsyncResult        *res,
			       GError             **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
	GHashTable *ret;

	g_return_val_if_fail (GEOCODE_IS_OBJECT (object), NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_object_resolve_async);

	ret = NULL;

	if (g_simple_async_result_propagate_error (simple, error))
		goto out;

	ret = g_simple_async_result_get_op_res_gpointer (simple);

out:
	return ret;
}

/**
 * geocode_get_geocode:
 * @object: a #GeocodeObject representing a query
 * @error: a #GError
 *
 * Gets the result of a geocoding or reverse geocoding
 * query using a web service.
 *
 * Returns: a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GHashTable *
geocode_object_resolve (GeocodeObject       *object,
			GError             **error)
{
	GFile *query;
	char *contents;
	GHashTable *ret;

	g_return_val_if_fail (GEOCODE_IS_OBJECT (object), NULL);

	query = get_query_for_params (object);
	if (g_file_load_contents (query,
				  NULL,
				  &contents,
				  NULL,
				  NULL,
				  error) == FALSE) {
		return NULL;
	}
	g_object_unref (query);

	ret = _geocode_parse_json (contents, error);

	g_free (contents);

	return ret;
}
