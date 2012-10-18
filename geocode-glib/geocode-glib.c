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

#include <string.h>
#include <errno.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <geocode-glib.h>
#include <geocode-error.h>
#include <geocode-glib-private.h>

/**
 * SECTION:geocode-glib
 * @short_description: Geocode glib main functions
 * @include: geocode-glib/geocode-glib.h
 *
 * Contains functions for geocoding and reverse geocoding using the
 * <ulink url="http://developer.yahoo.com/geo/placefinder/guide/requests.html">Yahoo! Place Finder APIs</ulink>.
 * Lookups will only ever return one result, or an error if the lookup failed.
 **/

struct _GeocodeObjectPrivate {
	GHashTable *ht;
	guint reverse : 1;
	guint flags_added : 1;
};

G_DEFINE_TYPE (GeocodeObject, geocode_object, G_TYPE_OBJECT)

static void
geocode_object_finalize (GObject *gobject)
{
	GeocodeObject *object = (GeocodeObject *) gobject;

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

static char *
geocode_object_cache_path_for_query (GFile *query)
{
	const char *filename;
	char *path;
	char *uri;
	GChecksum *sum;

	/* Create cache directory */
	path = g_build_filename (g_get_user_cache_dir (),
				 "geocode-glib",
				 NULL);
	if (g_mkdir_with_parents (path, 0700) < 0) {
		g_warning ("Failed to mkdir path '%s': %s", path, g_strerror (errno));
		g_free (path);
		return NULL;
	}
	g_free (path);

	/* Create path for query */
	uri = g_file_get_uri (query);

	sum = g_checksum_new (G_CHECKSUM_SHA256);
	g_checksum_update (sum, (const guchar *) uri, strlen (uri));

	filename = g_checksum_get_string (sum);

	path = g_build_filename (g_get_user_cache_dir (),
				 "geocode-glib",
				 filename,
				 NULL);

	g_checksum_free (sum);
	g_free (uri);

	return path;
}

static gboolean
geocode_object_cache_save (GFile      *query,
			   const char *contents)
{
	char *path;
	gboolean ret;

	path = geocode_object_cache_path_for_query (query);
	ret = g_file_set_contents (path, contents, -1, NULL);

	g_free (path);
	return ret;
}

static gboolean
geocode_object_cache_load (GFile  *query,
			   char  **contents)
{
	char *path;
	gboolean ret;

	path = geocode_object_cache_path_for_query (query);
	ret = g_file_get_contents (path, contents, NULL, NULL);

	g_free (path);
	return ret;
}

struct {
	const char *tp_attr;
	const char *gc_attr; /* NULL to ignore */
} attrs_map[] = {
	{ "countrycode", NULL },
	{ "country", "country" },
	{ "region", "state" },
	{ "locality", "city" },
	{ "area", "neighborhood" },
	{ "postalcode", "postal" },
	{ "street", "street" },
	{ "building", "house" },
	{ "floor", "" },
	{ "room", "unit" },
	{ "text", NULL },
	{ "description", NULL },
	{ "uri", NULL },
	{ "language", NULL }, /* FIXME: Should we ignore this? */
};

/**
 * geocode_object_new_for_params:
 * @params: a #GHashTable with string keys, and #GValue values.
 *
 * Creates a new #GeocodeObject to perform geocoding with. The
 * #GHashTable is in the format used by Telepathy, and documented
 * on <ulink url="http://telepathy.freedesktop.org/spec/Connection_Interface_Location.html#Mapping:Location">Telepathy's specification site</ulink>.
 *
 * See also: <ulink url="http://xmpp.org/extensions/xep-0080.html">XEP-0080 specification</ulink>.
 *
 * Returns: a new #GeocodeObject. Use g_object_unref() when done.
 **/
GeocodeObject *
geocode_object_new_for_params (GHashTable *params)
{
	GeocodeObject *object;
	guint i;

	g_return_val_if_fail (params != NULL, NULL);

	if (g_hash_table_lookup (params, "lat") != NULL &&
	    g_hash_table_lookup (params, "long") != NULL) {
		g_warning ("You already have longitude and latitude in those parameters");
		return NULL;
	}

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);

	for (i = 0; i < G_N_ELEMENTS (attrs_map); i++) {
		GValue *value;

		if (attrs_map[i].gc_attr == NULL)
			continue;

		value = g_hash_table_lookup (params, attrs_map[i].tp_attr);
		if (value == NULL)
			continue;

		geocode_object_add (object,
				    attrs_map[i].gc_attr,
				    g_value_get_string (value));
	}

	return object;
}

/**
 * geocode_object_new_for_coords:
 * @latitude: a valid latitude
 * @longitude: a valid longitude
 *
 * Creates a new #GeocodeObject to perform reverse geocoding with.
 * Use geocode_object_resolve_async() to perform the resolution.
 *
 * Returns: a new #GeocodeObject. Use g_object_unref() when done.
 **/
GeocodeObject *
geocode_object_new_for_coords (gdouble     latitude,
			       gdouble     longitude)
{
	GeocodeObject *object;
	char buf[16], buf2[16];

	g_return_val_if_fail (longitude >= -180.0 && longitude <= 180.0, NULL);
	g_return_val_if_fail (latitude >= -90.0 && latitude <= 90.0, NULL);

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);
	object->priv->reverse = TRUE;

	g_hash_table_insert (object->priv->ht,
			     g_strdup ("location"),
			     g_strdup_printf ("%s, %s",
					      g_ascii_formatd (buf, sizeof (buf), "%g", latitude),
					      g_ascii_formatd (buf2, sizeof (buf2), "%g", longitude)));

	return object;
}

/**
 * geocode_object_new_for_location:
 * @location: a string containing a free-form description of the location
 *
 * Creates a new #GeocodeObject to perform geocoding with. The
 * string is in free-form format.
 *
 * Returns: a new #GeocodeObject. Use g_object_unref() when done.
 **/
GeocodeObject *
geocode_object_new_for_location (const char *location)
{
	GeocodeObject *object;

	g_return_val_if_fail (location != NULL, NULL);

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);
	geocode_object_add (object, "location", location);

	return object;
}

/**
 * geocode_object_add:
 * @object: a #GeocodeObject
 * @key: a string representing a parameter to the web service
 * @value: a string representing the value of a parameter
 *
 * Adds parameters to pass to the web service. A copy of the key
 * and value parameters are kept internally.
 **/
void
geocode_object_add (GeocodeObject *object,
		    const char    *key,
		    const char    *value)
{
	g_return_if_fail (GEOCODE_IS_OBJECT (object));
	g_return_if_fail (key != NULL);
	g_return_if_fail (value == NULL || g_utf8_validate (value, -1, NULL));

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
	gint64 err_code, found;
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
		case 100:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "No input parameters");
			break;
		case 102:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "Address data not recognized as valid UTF-8");
			break;
		case 103:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "Insufficient address data");
			break;
		case 104:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "Unknown language");
			break;
		case 105:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "No country detected");
			break;
		case 106:
			g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NOT_SUPPORTED, msg ? msg : "Country not supported");
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

	/* Check for the number of results */
	if (json_reader_read_member (reader, "Found") == FALSE)
		goto parse;

	found = json_reader_get_int_value (reader);
	json_reader_end_member (reader);

	if (!found) {
		g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_NO_MATCHES, "No matches found for request");
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

	if (ret == NULL) {
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		g_error_free (error);
		g_free (contents);
		return;
	}

	/* Now that we can parse the result, save it to cache */
	geocode_object_cache_save (query, contents);
	g_free (contents);

	g_simple_async_result_set_op_res_gpointer (simple, ret, NULL);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);
}

static void
on_cache_data_loaded (GObject      *source_object,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
	GCancellable *cancellable;
	GFile *cache;
	GError *error = NULL;
	char *contents;
	GHashTable *ret;

	cache = G_FILE (source_object);
	cancellable = g_object_get_data (G_OBJECT (cache), "cancellable");
	if (g_file_load_contents_finish (cache,
					 res,
					 &contents,
					 NULL,
					 NULL,
					 NULL) == FALSE) {
		GFile *query;

		query = g_object_get_data (G_OBJECT (cache), "query");
		g_file_load_contents_async (query,
					    cancellable,
					    on_query_data_loaded,
					    simple);
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
		geocode_object_add (object, "appid", YAHOO_APPID);
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
 * query using a web service. Use geocode_object_resolve() to do the same
 * thing synchronously.
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
	char *cache_path;

	g_return_if_fail (GEOCODE_IS_OBJECT (object));

	simple = g_simple_async_result_new (G_OBJECT (object),
					    callback,
					    user_data,
					    geocode_object_resolve_async);

	query = get_query_for_params (object);
	cache_path = geocode_object_cache_path_for_query (query);
	if (cache_path == NULL) {
		g_file_load_contents_async (query,
					    cancellable,
					    on_query_data_loaded,
					    simple);
		g_object_unref (query);
	} else {
		GFile *cache;

		cache = g_file_new_for_path (cache_path);
		g_object_set_data_full (G_OBJECT (cache), "query", query, (GDestroyNotify) g_object_unref);
		g_object_set_data (G_OBJECT (cache), "cancellable", cancellable);
		g_file_load_contents_async (cache,
					    cancellable,
					    on_cache_data_loaded,
					    simple);
		g_object_unref (cache);
	}
}

/**
 * geocode_object_resolve_finish:
 * @object: a #GeocodeObject representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a query operation. See geocode_object_resolve_async().
 *
 * Returns: (element-type utf8 utf8) (transfer full):
 * a #GHashTable containing the results of the query
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
 * geocode_object_resolve:
 * @object: a #GeocodeObject representing a query
 * @error: a #GError
 *
 * Gets the result of a geocoding or reverse geocoding
 * query using a web service.
 *
 * Returns: (element-type utf8 utf8) (transfer full):
 * a #GHashTable containing the results of the query
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
	gboolean to_cache = FALSE;

	g_return_val_if_fail (GEOCODE_IS_OBJECT (object), NULL);

	query = get_query_for_params (object);
	if (geocode_object_cache_load (query, &contents) == FALSE) {
		if (g_file_load_contents (query,
					  NULL,
					  &contents,
					  NULL,
					  NULL,
					  error) == FALSE) {
			g_object_unref (query);
			return NULL;
		}
		to_cache = TRUE;
	}

	ret = _geocode_parse_json (contents, error);
	if (to_cache)
		geocode_object_cache_save (query, contents);

	g_free (contents);
	g_object_unref (query);

	return ret;
}

/**
 * geocode_object_get_coords:
 * @results: a #GHashTable as generated by geocode_object_resolve()
 * and geocode_object_resolve_async()
 * @longitude: a pointer to a #gdouble
 * @latitude: a pointer to a #gdouble
 *
 * Parses the results hashtable and exports the longitude and latitude
 * as #gdoubles.
 *
 * Returns: %TRUE when the longitude and latitude have been
 * found and the return parameters set.
 **/
gboolean
geocode_object_get_coords (GHashTable *results,
			   gdouble    *longitude,
			   gdouble    *latitude)
{
	const char *s;
	gboolean ret = TRUE;

	g_return_val_if_fail (results != NULL, FALSE);

	s = g_hash_table_lookup (results, "longitude");
	if (s == NULL)
		ret = FALSE;
	else if (longitude)
		*longitude = g_strtod (s, NULL);

	s = g_hash_table_lookup (results, "latitude");
	if (s == NULL)
		ret = FALSE;
	else if (latitude)
		 *latitude = g_strtod (s, NULL);

	return ret;
}

