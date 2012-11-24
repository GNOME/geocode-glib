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
#include <stdlib.h>
#include <locale.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <geocode-glib/geocode-forward.h>
#include <geocode-glib/geocode-error.h>
#include <geocode-glib/geocode-glib-private.h>

/**
 * SECTION:geocode-forward
 * @short_description: Geocode forward geocoding object
 * @include: geocode-glib/geocode-glib.h
 *
 * Contains functions for forward geocoding using the
 * <ulink url="http://developer.yahoo.com/geo/geoplanet/">Yahoo! GeoPlanet APIs</ulink>.
 **/

struct _GeocodeForwardPrivate {
	GHashTable *ht;
	guint       answer_count;
};

G_DEFINE_TYPE (GeocodeForward, geocode_forward, G_TYPE_OBJECT)

#define DEFAULT_ANSWER_COUNT 10

static void geocode_forward_add (GeocodeForward *forward,
				 const char     *key,
				 const char     *value);
static GList *__geocode_parse_search_json (const char *contents,
					  GError    **error);

static void
geocode_forward_finalize (GObject *gforward)
{
	GeocodeForward *forward = (GeocodeForward *) gforward;

	g_clear_pointer (&forward->priv->ht, g_hash_table_destroy);

	G_OBJECT_CLASS (geocode_forward_parent_class)->finalize (gforward);
}

static void
geocode_forward_class_init (GeocodeForwardClass *klass)
{
	GObjectClass *gforward_class = G_OBJECT_CLASS (klass);

	gforward_class->finalize = geocode_forward_finalize;

	g_type_class_add_private (klass, sizeof (GeocodeForwardPrivate));
}

static void
geocode_forward_init (GeocodeForward *forward)
{
	forward->priv = G_TYPE_INSTANCE_GET_PRIVATE ((forward), GEOCODE_TYPE_FORWARD, GeocodeForwardPrivate);
	forward->priv->ht = g_hash_table_new_full (g_str_hash, g_str_equal,
						   g_free, g_free);
	forward->priv->answer_count = DEFAULT_ANSWER_COUNT;
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
geocode_forward_get_lang_for_locale (const char *locale)
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
geocode_forward_get_lang (void)
{
	return geocode_forward_get_lang_for_locale (setlocale (LC_MESSAGES, NULL));
}

static struct {
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
geocode_forward_fill_params (GeocodeForward *forward,
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

		geocode_forward_add (forward,
				    attrs_map[i].gc_attr,
				    str);
	}
}

/**
 * geocode_forward_new_for_params:
 * @params: (transfer none) (element-type utf8 GValue): a #GHashTable with string keys, and #GValue values.
 *
 * Creates a new #GeocodeForward to perform geocoding with. The
 * #GHashTable is in the format used by Telepathy, and documented
 * on <ulink url="http://telepathy.freedesktop.org/spec/Connection_Interface_Location.html#Mapping:Location">Telepathy's specification site</ulink>.
 *
 * See also: <ulink url="http://xmpp.org/extensions/xep-0080.html">XEP-0080 specification</ulink>.
 *
 * Returns: a new #GeocodeForward. Use g_object_unref() when done.
 **/
GeocodeForward *
geocode_forward_new_for_params (GHashTable *params)
{
	GeocodeForward *forward;

	g_return_val_if_fail (params != NULL, NULL);

	if (g_hash_table_lookup (params, "lat") != NULL &&
	    g_hash_table_lookup (params, "long") != NULL) {
		g_warning ("You already have longitude and latitude in those parameters");
	}

	forward = g_object_new (GEOCODE_TYPE_FORWARD, NULL);
	geocode_forward_fill_params (forward, params, FALSE);

	return forward;
}

/**
 * geocode_forward_new_for_string:
 * @str: a string containing a free-form description of the location
 *
 * Creates a new #GeocodeForward to perform geocoding with. The
 * string is in free-form format.
 *
 * Returns: a new #GeocodeForward. Use g_object_unref() when done.
 **/
GeocodeForward *
geocode_forward_new_for_string (const char *location)
{
	GeocodeForward *forward;

	g_return_val_if_fail (location != NULL, NULL);

	forward = g_object_new (GEOCODE_TYPE_FORWARD, NULL);
	geocode_forward_add (forward, "location", location);

	return forward;
}

/**
 * geocode_forward_add:
 * @forward: a #GeocodeForward
 * @key: a string representing a parameter to the web service
 * @value: a string representing the value of a parameter
 *
 * Adds parameters to the geocoding or reverse geocoding request.
 * A copy of the key and value parameters are kept internally.
 *
 * This function is mainly intended for language bindings.
 **/
static void
geocode_forward_add (GeocodeForward *forward,
		     const char    *key,
		     const char    *value)
{
	g_return_if_fail (GEOCODE_IS_FORWARD (forward));
	g_return_if_fail (key != NULL);
	g_return_if_fail (value == NULL || g_utf8_validate (value, -1, NULL));

	g_hash_table_insert (forward->priv->ht,
			     g_strdup (key),
			     g_strdup (value));
}

static void
on_query_data_loaded (GObject      *source_forward,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
	GFile *query;
	GError *error = NULL;
	char *contents;
	gpointer ret;

	query = G_FILE (source_forward);
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

	ret = __geocode_parse_search_json (contents, &error);

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
on_cache_data_loaded (GObject      *source_forward,
		      GAsyncResult *res,
		      gpointer      user_data)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
	GCancellable *cancellable;
	GFile *cache;
	GError *error = NULL;
	char *contents;
	gpointer ret;

	cache = G_FILE (source_forward);
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

	ret = __geocode_parse_search_json (contents, &error);
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
get_search_query_for_params (GeocodeForward *forward,
			     GError        **error)
{
	GFile *ret;
	GHashTable *ht;
	char *lang;
	char *params;
	char *search_term;
	char *uri;
	const char *location;

	location = g_hash_table_lookup (forward->priv->ht, "location");
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
	lang = geocode_forward_get_lang ();
	if (lang)
		g_hash_table_insert (ht, "lang", lang);

	params = soup_form_encode_hash (ht);
	g_hash_table_destroy (ht);
	g_free (lang);

	uri = g_strdup_printf ("http://where.yahooapis.com/v1/places.q('%s');start=0;count=%u?%s",
			       search_term, forward->priv->answer_count, params);
	g_free (params);
	g_free (search_term);

	ret = g_file_new_for_uri (uri);
	g_free (uri);

	return ret;
}

/**
 * geocode_forward_search_async:
 * @forward: a #GeocodeForward representing a query
 * @cancellable: optional #GCancellable forward, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously gets a list of a geocoding
 * query using a web service. Use geocode_forward_search() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_forward_search_finish() to get the result of the operation.
 **/
void
geocode_forward_search_async (GeocodeForward       *forward,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      gpointer             user_data)
{
	GSimpleAsyncResult *simple;
	GFile *query;
	char *cache_path;
	GError *error = NULL;

	g_return_if_fail (GEOCODE_IS_FORWARD (forward));

	simple = g_simple_async_result_new (G_OBJECT (forward),
					    callback,
					    user_data,
					    geocode_forward_search_async);

	query = get_search_query_for_params (forward, &error);
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
 * geocode_forward_search_finish:
 * @forward: a #GeocodeForward representing a query
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes a query operation. See geocode_forward_search_async().
 *
 * Returns: (element-type GHashTable) (transfer full):
 * a #GHashTable containing the results of the query
 * or %NULL in case of errors.
 * Free the returned string with g_hash_table_destroy() when done.
 **/
GList *
geocode_forward_search_finish (GeocodeForward       *forward,
			       GAsyncResult        *res,
			       GError             **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
	GList *ret;

	g_return_val_if_fail (GEOCODE_IS_FORWARD (forward), NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_forward_search_async);

	ret = NULL;

	if (g_simple_async_result_propagate_error (simple, error))
		goto out;

	ret = g_simple_async_result_get_op_res_gpointer (simple);

out:
	return ret;
}

#define IS_EL(x) (g_str_equal (element_name, x))

static void
insert_place_attr (GHashTable *ht,
		   JsonReader *reader,
		   const char *element_name)
{
	char *value;

	if (json_reader_read_member (reader, element_name) == FALSE) {
		json_reader_end_member (reader);
		return;
	}

	/* FIXME: check all the member names against what Place Finder outputs */

	if (IS_EL("woeid") ||
	    IS_EL("popRank") ||
	    IS_EL("areaRank")) {
		value = g_strdup_printf ("%"G_GINT64_FORMAT, json_reader_get_int_value (reader));
	} else if (IS_EL("centroid")) {
		json_reader_read_member (reader, "longitude");
		g_hash_table_insert (ht, g_strdup ("longitude"),
				     g_strdup_printf ("%lf", json_reader_get_double_value (reader)));
		json_reader_end_member (reader);
		json_reader_read_member (reader, "latitude");
		g_hash_table_insert (ht, g_strdup ("latitude"),
				     g_strdup_printf ("%lf", json_reader_get_double_value (reader)));
		json_reader_end_member (reader);
		goto end;
	} else if (g_str_has_suffix (element_name, " attrs")) {
		g_debug ("Ignoring attributes element '%s'", element_name);
		value = g_strdup (""); /* So that they're ignored */
	} else if (IS_EL("boundingBox")) {
		g_debug ("Ignoring element '%s'", element_name);
		value = g_strdup (""); /* So that they're ignored */
	} else {
		value = g_strdup (json_reader_get_string_value (reader));
	}

	if (value != NULL && *value == '\0') {
		g_clear_pointer (&value, g_free);
		goto end;
	}

	if (value != NULL)
		g_hash_table_insert (ht, g_strdup (element_name), value);
	else
		g_warning ("Ignoring element %s, don't know how to parse it", element_name);

end:
	json_reader_end_member (reader);
}

static char *
create_description_from_attrs (GHashTable *ht,
			       gdouble    *longitude,
			       gdouble    *latitude)
{
	*longitude = strtod (g_hash_table_lookup (ht, "longitude"), NULL);
	*latitude = strtod (g_hash_table_lookup (ht, "latitude"), NULL);
	return g_strdup (g_hash_table_lookup (ht, "name"));
}

static GeocodeLocation *
new_location_from_result (GHashTable *ht)
{
	GeocodeLocation *loc;
	char *description;
	gdouble longitude, latitude;

	description = create_description_from_attrs (ht,
						     &longitude,
						     &latitude);
	loc = geocode_location_new_with_description (longitude,
						     latitude,
						     description);
	g_free (description);

	return loc;
}

static GList *
__geocode_parse_search_json (const char *contents,
			     GError    **error)
{
	GList *ret;
	JsonParser *parser;
	JsonNode *root;
	JsonReader *reader;
	const GError *err = NULL;
	int num_places, i;

	ret = NULL;

	parser = json_parser_new ();
	if (json_parser_load_from_data (parser, contents, -1, error) == FALSE) {
		g_object_unref (parser);
		return ret;
	}

	root = json_parser_get_root (parser);
	reader = json_reader_new (root);

	if (json_reader_read_member (reader, "places") == FALSE)
		goto parse;
	if (json_reader_read_member (reader, "place") == FALSE)
		goto parse;

	num_places = json_reader_count_elements (reader);
	if (num_places < 0)
		goto parse;

	for (i = 0; i < num_places; i++) {
		GeocodeLocation *loc;
		GHashTable *ht;
		char **members;
		int j;

		json_reader_read_element (reader, i);

		ht = g_hash_table_new_full (g_str_hash, g_str_equal,
					    g_free, g_free);

		members = json_reader_list_members (reader);
		for (j = 0; members != NULL && members[j] != NULL; j++)
			insert_place_attr (ht, reader, members[j]);
		g_strfreev (members);

		json_reader_end_element (reader);

		loc = new_location_from_result (ht);

		ret = g_list_prepend (ret, loc);
	}

	g_object_unref (parser);
	g_object_unref (reader);
	ret = g_list_reverse (ret);

	return ret;
parse:
	err = json_reader_get_error (reader);
	g_set_error_literal (error, GEOCODE_ERROR, GEOCODE_ERROR_PARSE, err->message);
	g_object_unref (parser);
	g_object_unref (reader);
	return NULL;
}

/**
 * geocode_forward_search:
 * @forward: a #GeocodeForward representing a query
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
geocode_forward_search (GeocodeForward      *forward,
			GError             **error)
{
	GFile *query;
	char *contents;
	GList *ret;
	gboolean to_cache = FALSE;

	g_return_val_if_fail (GEOCODE_IS_FORWARD (forward), NULL);

	query = get_search_query_for_params (forward, error);
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

	ret = __geocode_parse_search_json (contents, error);
	if (to_cache && ret != NULL)
		_geocode_glib_cache_save (query, contents);

	g_free (contents);
	g_object_unref (query);

	return ret;
}

void
geocode_forward_set_answer_count (GeocodeForward *forward,
				  guint           count)
{
	g_return_if_fail (GEOCODE_IS_FORWARD (forward));

	/* FIXME: make a property */
	forward->priv->answer_count = count;
}
