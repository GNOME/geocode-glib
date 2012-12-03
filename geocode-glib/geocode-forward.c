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

static void geocode_forward_add (GeocodeForward *forward,
				 const char     *key,
				 const char     *value);

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

static void
set_is_search (GeocodeForward *forward,
	       GObject        *object)
{
	if (forward->priv->answer_count != 1)
		g_object_set_data (object, "is-search", GINT_TO_POINTER (1));
}

static gboolean
is_search (GObject *object)
{
	return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object), "is-search"));
}

static GList *
_geocode_parse_single_result_json (const char  *contents,
				   GError     **error)
{
	GList *ret;
	GHashTable *ht;
	GeocodeLocation *loc;
	gdouble longitude;
	gdouble latitude;

	ht = _geocode_parse_resolve_json (contents, error);
	if (ht == NULL)
		return NULL;

	longitude = g_ascii_strtod (g_hash_table_lookup (ht, "longitude"), NULL);
	latitude = g_ascii_strtod (g_hash_table_lookup (ht, "latitude"), NULL);
	loc = geocode_location_new_with_description (longitude, latitude,
						     g_hash_table_lookup (ht, "line2"));

	ret = g_list_append (NULL, loc);

	return ret;
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
	{ "floor", NULL },
	{ "room", "unit" },
	{ "text", NULL },
	{ "description", NULL },
	{ "uri", NULL },
	{ "language", "locale" },
};

static const char *
tp_attr_to_gc_attr (const char *attr,
		    gboolean   *found)
{
	guint i;

	*found = FALSE;

	for (i = 0; i < G_N_ELEMENTS (attrs_map); i++) {
		if (g_str_equal (attr, attrs_map[i].tp_attr)){
			*found = TRUE;
			return attrs_map[i].gc_attr;
		}
	}

	return NULL;
}

static void
geocode_forward_fill_params (GeocodeForward *forward,
			     GHashTable    *params)
{
	GHashTableIter iter;
	GValue *value;
	const char *key;

	g_hash_table_iter_init (&iter, params);
	while (g_hash_table_iter_next (&iter, (gpointer *) &key, (gpointer *) &value)) {
		gboolean found;
		const char *gc_attr;
		const char *str;

		gc_attr = tp_attr_to_gc_attr (key, &found);
		if (found == FALSE) {
			g_warning ("XEP attribute '%s' unhandled", key);
			continue;
		}
		if (gc_attr == NULL)
			continue;

		str = g_value_get_string (value);
		if (str == NULL)
			continue;

		geocode_forward_add (forward, gc_attr, str);
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
 * Note that you will get exactly one result when doing the search.
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
	geocode_forward_fill_params (forward, params);
	geocode_forward_set_answer_count (forward, 1);

	return forward;
}

/**
 * geocode_forward_new_for_string:
 * @str: a string containing a free-form description of the location
 *
 * Creates a new #GeocodeForward to perform forward geocoding with. The
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

	if (is_search (G_OBJECT (query)))
		ret = _geocode_parse_search_json (contents, &error);
	else
		ret = _geocode_parse_single_result_json (contents, &error);

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
		g_object_set_data (G_OBJECT (query), "is-search",
				   g_object_get_data (G_OBJECT (cache), "is-search"));
		g_file_load_contents_async (query,
					    cancellable,
					    on_query_data_loaded,
					    simple);
		return;
	}

	if (is_search (G_OBJECT (cache)))
		ret = _geocode_parse_search_json (contents, &error);
	else
		ret = _geocode_parse_single_result_json (contents, &error);
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
	lang = _geocode_object_get_lang ();
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
 * Asynchronously performs a forward geocoding
 * query using a web service. Use geocode_forward_search() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_forward_search_finish() to get the result of the operation.
 **/
void
geocode_forward_search_async (GeocodeForward      *forward,
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

	if (forward->priv->answer_count != 1)
		query = get_search_query_for_params (forward, &error);
	else
		query = _get_resolve_query_for_params (forward->priv->ht, FALSE);
	if (!query) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		return;
	}

	cache_path = _geocode_glib_cache_path_for_query (query);
	if (cache_path == NULL) {
		set_is_search (forward, G_OBJECT (query));
		g_file_load_contents_async (query,
					    cancellable,
					    on_query_data_loaded,
					    simple);
		g_object_unref (query);
	} else {
		GFile *cache;

		cache = g_file_new_for_path (cache_path);
		set_is_search (forward, G_OBJECT (cache));
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
 * Finishes a forward geocoding operation. See geocode_forward_search_async().
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
		char str[16];

		json_reader_read_member (reader, "longitude");
		g_ascii_dtostr (str, sizeof(str), json_reader_get_double_value (reader));
		g_hash_table_insert (ht, g_strdup ("longitude"), g_strdup (str));
		json_reader_end_member (reader);

		json_reader_read_member (reader, "latitude");
		g_ascii_dtostr (str, sizeof(str), json_reader_get_double_value (reader));
		g_hash_table_insert (ht, g_strdup ("latitude"), g_strdup (str));
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

static gboolean
node_free_func (GNode    *node,
		gpointer  user_data)
{
	/* Leaf nodes are GeocodeLocation objects
	 * which we reuse for the results */
	if (G_NODE_IS_LEAF (node) == FALSE)
		g_free (node->data);

	return FALSE;
}

#define N_ATTRS 7
static const char const *attributes[7] = {
	"country",
	"admin1",
	"admin2",
	"admin3",
	"postal",
	"placeTypeName",
	"locality1"
};

static void
insert_place_into_tree (GNode *location_tree, GHashTable *ht)
{
	GNode *start = location_tree, *child = NULL;
	GeocodeLocation *loc = NULL;
	char *attr_val = NULL;
	char *name;
	gdouble longitude, latitude;
	guint i;

	for (i = 0; i < G_N_ELEMENTS(attributes); i++) {
		attr_val = g_hash_table_lookup (ht, attributes[i]);
		if (!attr_val) {
			/* Add a dummy node if the attribute value is not
			 * available for the place */
			child = g_node_insert_data (start, -1, NULL);
		} else {
			/* If the attr value (eg for country United States)
			 * already exists, then keep on adding other attributes under that node. */
			child = g_node_first_child (start);
			while (child &&
			       child->data &&
			       g_ascii_strcasecmp (child->data, attr_val) != 0) {
				child = g_node_next_sibling (child);
			}
			if (!child) {
				/* create a new node */
				child = g_node_insert_data (start, -1, g_strdup (attr_val));
			}
		}
		start = child;
	}

	/* Get latitude and longitude and create GeocodeLocation object.
	 * The leaf node of the tree is the GeocodeLocation object */
	longitude = g_ascii_strtod (g_hash_table_lookup (ht, "longitude"), NULL);
	latitude = g_ascii_strtod (g_hash_table_lookup (ht, "latitude"), NULL);
	name = g_hash_table_lookup (ht, "name");

	loc = geocode_location_new_with_description (latitude, longitude, name);

	g_node_insert_data (start, -1, loc);
}

static void
make_location_list_from_tree (GNode   *node,
			      char   **s_array,
			      GList  **location_list,
			      int      i)
{
	GNode *child;
	GeocodeLocation *loc;
	char *description, *name;
	int counter = 0;
	gboolean add_attribute = FALSE;

	if (node == NULL)
		return;

	if (G_NODE_IS_LEAF (node)) {
		GPtrArray *rev_s_array;

		rev_s_array = g_ptr_array_new ();

		/* If leaf node, then add all the attributes in the s_array
		 * and set it to the description of the loc object */
		loc = (GeocodeLocation *) node->data;

		name = loc->description;

		/* To print the attributes in a meaningful manner
		 * reverse the s_array */
		g_ptr_array_add (rev_s_array, name);
		counter = 1;
		while (counter <= i) {
			g_ptr_array_add (rev_s_array, s_array[i - counter]);
			counter++;
		}
		g_ptr_array_add (rev_s_array, NULL);
		description = g_strjoinv (", ", (char **) rev_s_array->pdata);
		g_ptr_array_unref (rev_s_array);

		loc->description = description;
		g_free (name);

		/* no need to free description since when GeocodeLocation
		 * object gets freed it will free the description */
		*location_list = g_list_prepend (*location_list, loc);
	} else {
		/* If there are other attributes with a different value,
		 * add those attributes to the string to differentiate them */
		if (g_node_prev_sibling (node) ||
		   g_node_next_sibling (node))
			add_attribute = TRUE;

		if (add_attribute) {
			s_array[i] = node->data;
			i++;
		}
	}

	child = node->children;
	while (child) {
		make_location_list_from_tree (child, s_array, location_list, i);
		child = child->next;
	}
	if (add_attribute)
		i--;
}

GList *
_geocode_parse_search_json (const char *contents,
			     GError    **error)
{
	GList *ret;
	JsonParser *parser;
	JsonNode *root;
	JsonReader *reader;
	const GError *err = NULL;
	int num_places, i;
	GNode *location_tree;
	char *s_array[N_ATTRS];

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

	location_tree = g_node_new (NULL);

	for (i = 0; i < num_places; i++) {
		GHashTable *ht;
		char **members;
		int j;

		json_reader_read_element (reader, i);

		ht = g_hash_table_new_full (g_str_hash, g_str_equal,
					    g_free, g_free);

		members = json_reader_list_members (reader);
		for (j = 0; members != NULL && members[j] != NULL; j++)
			insert_place_attr (ht, reader, members[j]);

		/* Populate the tree with place details */
		insert_place_into_tree (location_tree, ht);

		g_hash_table_destroy (ht);
		g_strfreev (members);

		json_reader_end_element (reader);
	}

	make_location_list_from_tree (location_tree, s_array, &ret, 0);

	g_node_traverse (location_tree,
			 G_IN_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) node_free_func,
			 NULL);

	g_node_destroy (location_tree);

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
 * Gets the result of a forward geocoding
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

	if (forward->priv->answer_count != 1)
		query = get_search_query_for_params (forward, error);
	else
		query = _get_resolve_query_for_params (forward->priv->ht, FALSE);

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

	if (forward->priv->answer_count != 1)
		ret = _geocode_parse_search_json (contents, error);
	else
		ret = _geocode_parse_single_result_json (contents, error);
	if (to_cache && ret != NULL)
		_geocode_glib_cache_save (query, contents);

	g_free (contents);
	g_object_unref (query);

	return ret;
}

/**
 * geocode_forward_set_answer_count:
 * @forward: a #GeocodeForward representing a query
 * @count: the number of requested results
 *
 * Sets the number of requested results to @count.
 **/
void
geocode_forward_set_answer_count (GeocodeForward *forward,
				  guint           count)
{
	g_return_if_fail (GEOCODE_IS_FORWARD (forward));

	/* FIXME: make a property */
	forward->priv->answer_count = count;
}
