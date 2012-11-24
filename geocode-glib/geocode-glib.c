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
#include <locale.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <geocode-glib.h>
#include <geocode-error.h>
#include <geocode-glib-private.h>

#define IS_SEARCH "is-search"

/**
 * SECTION:geocode-glib
 * @short_description: Geocode glib main functions
 * @include: geocode-glib/geocode-glib.h
 *
 * Contains functions for geocoding and reverse geocoding using the
 * <ulink url="http://developer.yahoo.com/geo/placefinder/guide/requests.html">Yahoo! Place Finder APIs</ulink>
 * for geocoding and reverse geocoding, and the
 * <ulink url="http://developer.yahoo.com/geo/geoplanet/">Yahoo! GeoPlanet APIs</ulink>
 * for forward geocoding searches.
 **/

struct _GeocodeObjectPrivate {
	GHashTable *ht;
	GeocodeLookupType type;
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
	object->priv->type = GEOCODE_GLIB_RESOLVE_FORWARD;
	object->priv->ht = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, g_free);
}

char *
_geocode_glib_cache_path_for_query (GFile *query)
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

gboolean
_geocode_glib_cache_save (GFile      *query,
			  const char *contents)
{
	char *path;
	gboolean ret;

	path = _geocode_glib_cache_path_for_query (query);
	ret = g_file_set_contents (path, contents, -1, NULL);

	g_free (path);
	return ret;
}

gboolean
_geocode_glib_cache_load (GFile  *query,
			  char  **contents)
{
	char *path;
	gboolean ret;

	path = _geocode_glib_cache_path_for_query (query);
	ret = g_file_get_contents (path, contents, NULL, NULL);

	g_free (path);
	return ret;
}

static gboolean
parse_lang (const char *locale,
	    char      **language_codep,
	    char      **territory_codep)
{
	GRegex     *re;
	GMatchInfo *match_info;
	gboolean    res;
	GError     *error;
	gboolean    retval;

	match_info = NULL;
	retval = FALSE;

	error = NULL;
	re = g_regex_new ("^(?P<language>[^_.@[:space:]]+)"
			  "(_(?P<territory>[[:upper:]]+))?"
			  "(\\.(?P<codeset>[-_0-9a-zA-Z]+))?"
			  "(@(?P<modifier>[[:ascii:]]+))?$",
			  0, 0, &error);
	if (re == NULL) {
		g_warning ("%s", error->message);
		goto out;
	}

	if (!g_regex_match (re, locale, 0, &match_info) ||
	    g_match_info_is_partial_match (match_info)) {
		g_warning ("locale '%s' isn't valid\n", locale);
		goto out;
	}

	res = g_match_info_matches (match_info);
	if (! res) {
		g_warning ("Unable to parse locale: %s", locale);
		goto out;
	}

	retval = TRUE;

	*language_codep = g_match_info_fetch_named (match_info, "language");

	*territory_codep = g_match_info_fetch_named (match_info, "territory");

	if (*territory_codep != NULL &&
	    *territory_codep[0] == '\0') {
		g_free (*territory_codep);
		*territory_codep = NULL;
	}

out:
	g_match_info_free (match_info);
	g_regex_unref (re);

	return retval;
}

static char *
geocode_object_get_lang_for_locale (const char *locale)
{
	char *lang;
	char *territory;

	if (parse_lang (locale, &lang, &territory) == FALSE)
		return NULL;

	return g_strdup_printf ("%s%s%s",
				lang,
				territory ? "_" : "",
				territory ? territory : "");
}

static char *
geocode_object_get_lang (void)
{
	return geocode_object_get_lang_for_locale (setlocale (LC_MESSAGES, NULL));
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
	{ "language", "locale" },
};

static void
geocode_object_fill_params (GeocodeObject *object,
			    GHashTable    *params,
			    gboolean       value_is_str)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (attrs_map); i++) {
		const char *str;

		if (attrs_map[i].gc_attr == NULL)
			continue;

		if (value_is_str == FALSE) {
			GValue *value;

			value = g_hash_table_lookup (params, attrs_map[i].tp_attr);
			if (value == NULL)
				continue;

			str = g_value_get_string (value);
		} else {
			str = g_hash_table_lookup (params, attrs_map[i].tp_attr);
		}

		if (str == NULL)
			continue;

		geocode_object_add (object,
				    attrs_map[i].gc_attr,
				    str);
	}
}

/**
 * geocode_object_new_for_params:
 * @params: (transfer none) (element-type utf8 GValue): a #GHashTable with string keys, and #GValue values.
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

	g_return_val_if_fail (params != NULL, NULL);

	if (g_hash_table_lookup (params, "lat") != NULL &&
	    g_hash_table_lookup (params, "long") != NULL) {
		g_warning ("You already have longitude and latitude in those parameters");
	}

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);
	geocode_object_fill_params (object, params, FALSE);

	return object;
}

/**
 * geocode_object_new_for_params_str:
 * @params_str: (transfer none) (element-type utf8 utf8): a #GHashTable with string keys, and string values.
 *
 * Creates a new #GeocodeObject to perform geocoding with. The
 * #GHashTable uses the same keys used by Telepathy, and documented
 * on <ulink url="http://telepathy.freedesktop.org/spec/Connection_Interface_Location.html#Mapping:Location">Telepathy's specification site</ulink>,
 * with the difference that all the values are passed as strings.
 *
 * Returns: a new #GeocodeObject. Use g_object_unref() when done.
 **/
GeocodeObject *
geocode_object_new_for_params_str (GHashTable *params_str)
{
	GeocodeObject *object;

	g_return_val_if_fail (params_str != NULL, NULL);

	if (g_hash_table_lookup (params_str, "lat") != NULL &&
	    g_hash_table_lookup (params_str, "long") != NULL) {
		g_warning ("You already have longitude and latitude in those parameters");
	}

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);
	geocode_object_fill_params (object, params_str, TRUE);

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

	if (longitude < -180.0 || longitude > 180.0) {
		g_warning ("Invalid longitude %lf passed, using 0.0 instead", longitude);
		longitude = 0.0;
	}
	if (latitude < -90.0 || latitude > 90.0) {
		g_warning ("Invalid latitude %lf passed, using 0.0 instead", latitude);
		latitude = 0.0;
	}

	object = g_object_new (GEOCODE_TYPE_OBJECT, NULL);
	object->priv->type = GEOCODE_GLIB_RESOLVE_REVERSE;

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
 * Adds parameters to the geocoding or reverse geocoding request.
 * A copy of the key and value parameters are kept internally.
 *
 * This function is mainly intended for language bindings.
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

static gboolean
query_is_search (GFile *query)
{
	g_return_val_if_fail (query != NULL, FALSE);

	return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (query), IS_SEARCH));
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

	if (query_is_search (query) == FALSE)
		ret = _geocode_parse_resolve_json (contents, &error);
	else
		ret = _geocode_parse_search_json (contents, &error);

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

	if (query_is_search (cache) == FALSE)
		ret = _geocode_parse_resolve_json (contents, &error);
	else
		ret = _geocode_parse_search_json (contents, &error);
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

static GFile *
get_resolve_query_for_params (GeocodeObject *object,
			      GError       **error)
{
	GFile *ret;
	GHashTable *ht;
	char *locale;
	char *params, *uri;

	if (object->priv->type != GEOCODE_GLIB_RESOLVE_FORWARD &&
	    object->priv->type != GEOCODE_GLIB_RESOLVE_REVERSE) {
		g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_INVALID_ARGUMENTS,
				     "Arguments not setup for geocoding or reverse geocoding");
		return NULL;
	}

	ht = dup_ht (object->priv->ht);

	g_hash_table_insert (ht, "appid", YAHOO_APPID);
	g_hash_table_insert (ht, "flags", "QJT");

	if (object->priv->type == GEOCODE_GLIB_RESOLVE_REVERSE)
		g_hash_table_insert (ht, "gflags", "R");

	locale = NULL;
	if (g_hash_table_lookup (ht, "locale") == NULL) {
		locale = geocode_object_get_lang ();
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
	GError *error = NULL;

	g_return_if_fail (GEOCODE_IS_OBJECT (object));

	simple = g_simple_async_result_new (G_OBJECT (object),
					    callback,
					    user_data,
					    geocode_object_resolve_async);

	query = get_resolve_query_for_params (object, &error);
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

	query = get_resolve_query_for_params (object, error);
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

static GFile *
get_search_query_for_params (GeocodeObject *object,
			     GError       **error)
{
	GFile *ret;
	GHashTable *ht;
	char *lang;
	char *params;
	char *search_term;
	char *uri;
	const char *location;

	g_return_val_if_fail (object->priv->type == GEOCODE_GLIB_RESOLVE_FORWARD, NULL);
	location = g_hash_table_lookup (object->priv->ht, "location");
	if (location == NULL) {
		g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_INVALID_ARGUMENTS,
				     "No location argument set");
		return NULL;
	}

	/* Prepare the search term */
	search_term = soup_uri_encode (location, NULL);

	/* Prepare the query parameters */
	ht = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (ht, "appid", YAHOO_APPID);
	g_hash_table_insert (ht, "format", "json");
	lang = geocode_object_get_lang ();
	if (lang)
		g_hash_table_insert (ht, "lang", lang);

	params = soup_form_encode_hash (ht);
	g_hash_table_destroy (ht);
	g_free (lang);

	/* XXX: Make count a property? */
	uri = g_strdup_printf ("http://where.yahooapis.com/v1/places.q('%s');start=0;count=10?%s", search_term, params);
	g_free (params);
	g_free (search_term);

	ret = g_file_new_for_uri (uri);
	g_object_set_data (G_OBJECT (ret), IS_SEARCH, GINT_TO_POINTER (TRUE));
	g_free (uri);

	return ret;
}

/**
 * geocode_object_search_async:
 * @object: a #GeocodeObject representing a query
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously gets a list of a geocoding
 * query using a web service. Use geocode_object_search() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_object_search_finish() to get the result of the operation.
 **/
void
geocode_object_search_async (GeocodeObject       *object,
			     GCancellable        *cancellable,
			     GAsyncReadyCallback  callback,
			     gpointer             user_data)
{
	GSimpleAsyncResult *simple;
	GFile *query;
	char *cache_path;
	GError *error = NULL;

	g_return_if_fail (GEOCODE_IS_OBJECT (object));

	simple = g_simple_async_result_new (G_OBJECT (object),
					    callback,
					    user_data,
					    geocode_object_search_async);

	query = get_search_query_for_params (object, &error);
	if (!query) {
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
		g_object_set_data (G_OBJECT (cache), IS_SEARCH, GINT_TO_POINTER (TRUE));
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
 * geocode_object_search_finish:
 * @object: a #GeocodeObject representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a query operation. See geocode_object_search_async().
 *
 * Returns: (element-type GHashTable) (transfer full):
 * a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GList *
geocode_object_search_finish (GeocodeObject       *object,
			       GAsyncResult        *res,
			       GError             **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
	GList *ret;

	g_return_val_if_fail (GEOCODE_IS_OBJECT (object), NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_object_search_async);

	ret = NULL;

	if (g_simple_async_result_propagate_error (simple, error))
		goto out;

	ret = g_simple_async_result_get_op_res_gpointer (simple);

out:
	return ret;
}

/**
 * geocode_object_search:
 * @object: a #GeocodeObject representing a query
 * @error: a #GError
 *
 * Gets the result of a geocoding or reverse geocoding
 * query using a web service.
 *
 * Returns: (element-type GHashTable) (transfer full):
 * a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GList *
geocode_object_search (GeocodeObject       *object,
		       GError             **error)
{
	GFile *query;
	char *contents;
	GList *ret;
	gboolean to_cache = FALSE;

	g_return_val_if_fail (GEOCODE_IS_OBJECT (object), NULL);

	query = get_search_query_for_params (object, error);
	if (!query)
		return NULL;

	if (_geocode_glib_cache_load (query, &contents) == FALSE) {
		if (g_file_load_contents (query,
					  NULL,
					  &contents,
					  NULL,
					  NULL,
					  error) == FALSE) {
			/* FIXME check error value and match against
			 * web service errors:
			 * http://developer.yahoo.com/geo/geoplanet/guide/api_docs.html#response-errors */
			g_object_unref (query);
			return NULL;
		}
		to_cache = TRUE;
	}

	ret = _geocode_parse_search_json (contents, error);
	if (to_cache && ret != NULL)
		_geocode_glib_cache_save (query, contents);

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
