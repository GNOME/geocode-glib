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

#include <string.h>
#include <errno.h>
#include <locale.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <geocode-glib.h>
#include <geocode-error.h>
#include <geocode-reverse.h>
#include <geocode-glib-private.h>

/**
 * SECTION:geocode-reverse
 * @short_description: Geocode reverse geocoding object
 * @include: geocode-glib/geocode-glib.h
 *
 * Contains functions for reverse geocoding using the
 * <ulink url="http://developer.yahoo.com/geo/placefinder/guide/requests.html">Yahoo! Place Finder APIs</ulink>.
 **/

struct _GeocodeReversePrivate {
	GHashTable *ht;
};

G_DEFINE_TYPE (GeocodeReverse, geocode_reverse, G_TYPE_OBJECT)

static void
geocode_reverse_finalize (GObject *gobject)
{
	GeocodeReverse *object = (GeocodeReverse *) gobject;

	g_clear_pointer (&object->priv->ht, g_hash_table_destroy);

	G_OBJECT_CLASS (geocode_reverse_parent_class)->finalize (gobject);
}

static void
geocode_reverse_class_init (GeocodeReverseClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = geocode_reverse_finalize;

	g_type_class_add_private (klass, sizeof (GeocodeReversePrivate));
}

static void
geocode_reverse_init (GeocodeReverse *object)
{
	object->priv = G_TYPE_INSTANCE_GET_PRIVATE ((object), GEOCODE_TYPE_REVERSE, GeocodeReversePrivate);
	object->priv->ht = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, g_free);
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
	char buf[16], buf2[16];

	object = g_object_new (GEOCODE_TYPE_REVERSE, NULL);

	g_ascii_formatd (buf, sizeof (buf), "%g", geocode_location_get_latitude (location));
	g_ascii_formatd (buf2, sizeof (buf2), "%g", geocode_location_get_longitude (location));

	g_hash_table_insert (object->priv->ht,
			     g_strdup ("location"),
			     g_strdup_printf ("%s, %s", buf, buf2));

	return object;
}

static struct {
	const char *pf_attr;
	const char *xep_attr;
} attrs_map[] = {
	{ "longitude", "longitude" },
	{ "latitude", "latitude" },
	{ "offsetlat", NULL },
	{ "offsetlon", NULL },
	{ "name", "description" },
	{ "line1", "building" },
	{ "line2", NULL },
	{ "line3", NULL },
	{ "line4", NULL },
	{ "street", "street" },
	{ "postal", "postalcode" },
	{ "neighborhood", "area" },
	{ "city", "locality" },
	{ "county", NULL },
	{ "state", "region" },
	{ "country", "country" },
	{ "countrycode", "countrycode" },
	{ "countycode", NULL },
	{ "timezone", NULL },
	{ "uzip", NULL },
};

static const char *
pf_to_xep (const char *attr)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (attrs_map); i++) {
		if (g_str_equal (attr, attrs_map[i].pf_attr))
			return attrs_map[i].xep_attr;
	}

	g_debug ("Can't convert unknown attribute '%s'", attr);

	return NULL;
}

GHashTable *
_geocode_parse_resolve_json (const char *contents,
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
		    g_str_equal (members[i], "quality") ||
		    g_str_equal (members[i], "woeid")) {
			gint64 num;

			num = json_reader_get_int_value (reader);
			g_hash_table_insert (ret, g_strdup (members[i]), g_strdup_printf ("%"G_GINT64_FORMAT, num));
			json_reader_end_member (reader);
			continue;
		}

		value = json_reader_get_string_value (reader);
		if (value && *value == '\0')
			value = NULL;

		if (value != NULL) {
			const char *xep_attr;

			xep_attr = pf_to_xep (members[i]);
			if (xep_attr != NULL)
				g_hash_table_insert (ret, g_strdup (xep_attr), g_strdup (value));
			else
				g_hash_table_insert (ret, g_strdup (members[i]), g_strdup (value));
		}
		json_reader_end_member (reader);
	}
	g_strfreev (members);

	g_object_unref (parser);
	g_object_unref (reader);

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
	gpointer ret;

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

	ret = _geocode_parse_resolve_json (contents, &error);

	if (ret == NULL) {
		g_simple_async_result_set_from_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		g_error_free (error);
		g_free (contents);
		return;
	}

	/* Now that we can parse the result, save it to cache */
	_geocode_glib_cache_save (query, contents);
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
	gpointer ret;

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

	ret = _geocode_parse_resolve_json (contents, &error);
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

static void
copy_item (char       *key,
	   char       *value,
	   GHashTable *ret)
{
	g_hash_table_insert (ret, key, value);
}

static GHashTable *
dup_ht (GHashTable *ht)
{
	GHashTable *ret;

	ret = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_foreach (ht, (GHFunc) copy_item, ret);

	return ret;
}

GFile *
_get_resolve_query_for_params (GHashTable  *orig_ht,
			      gboolean     reverse)
{
	GFile *ret;
	GHashTable *ht;
	char *locale;
	char *params, *uri;

	ht = dup_ht (orig_ht);

	g_hash_table_insert (ht, "appid", YAHOO_APPID);
	g_hash_table_insert (ht, "flags", "QJT");
	if (reverse)
		g_hash_table_insert (ht, "gflags", "R");

	locale = NULL;
	if (g_hash_table_lookup (ht, "locale") == NULL) {
		locale = _geocode_object_get_lang ();
		if (locale)
			g_hash_table_insert (ht, "locale", locale);
	}

	params = soup_form_encode_hash (ht);
	g_hash_table_destroy (ht);
	g_free (locale);

	uri = g_strdup_printf ("http://where.yahooapis.com/geocode?%s", params);
	g_free (params);

	ret = g_file_new_for_uri (uri);
	g_free (uri);

	return ret;
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
geocode_reverse_resolve_async (GeocodeReverse       *object,
			       GCancellable        *cancellable,
			       GAsyncReadyCallback  callback,
			       gpointer             user_data)
{
	GSimpleAsyncResult *simple;
	GFile *query;
	char *cache_path;
	GError *error = NULL;

	g_return_if_fail (GEOCODE_IS_REVERSE (object));

	simple = g_simple_async_result_new (G_OBJECT (object),
					    callback,
					    user_data,
					    geocode_reverse_resolve_async);

	query = _get_resolve_query_for_params (object->priv->ht, TRUE);
	if (query == NULL) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		return;
	}

	cache_path = _geocode_glib_cache_path_for_query (query);
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
 * geocode_reverse_resolve_finish:
 * @object: a #GeocodeReverse representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a reverse geocoding operation. See geocode_reverse_resolve_async().
 *
 * Returns: (element-type utf8 utf8) (transfer full):
 * a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GHashTable *
geocode_reverse_resolve_finish (GeocodeReverse      *object,
				GAsyncResult        *res,
				GError             **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);

	g_return_val_if_fail (GEOCODE_IS_REVERSE (object), NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_reverse_resolve_async);

	if (g_simple_async_result_propagate_error (simple, error))
		return NULL;

	return g_simple_async_result_get_op_res_gpointer (simple);
}

/**
 * geocode_reverse_resolve:
 * @object: a #GeocodeReverse representing a query
 * @error: a #GError
 *
 * Gets the result of a reverse geocoding
 * query using a web service.
 *
 * Returns: (element-type utf8 utf8) (transfer full):
 * a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GHashTable *
geocode_reverse_resolve (GeocodeReverse      *object,
			 GError             **error)
{
	GFile *query;
	char *contents;
	GHashTable *ret;
	gboolean to_cache = FALSE;

	g_return_val_if_fail (GEOCODE_IS_REVERSE (object), NULL);

	query = _get_resolve_query_for_params (object->priv->ht, TRUE);
	if (query == NULL)
		return NULL;

	if (_geocode_glib_cache_load (query, &contents) == FALSE) {
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

	ret = _geocode_parse_resolve_json (contents, error);
	if (to_cache && ret != NULL)
		_geocode_glib_cache_save (query, contents);

	g_free (contents);
	g_object_unref (query);

	return ret;
}
