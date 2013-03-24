
#include "config.h"
#include <locale.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <geocode-glib.h>
#include <geocode-glib-private.h>

static GMainLoop *loop = NULL;

static char **params = NULL;

static void
print_loc (GeocodeLocation *loc)
{
	g_print ("\t%s @ %lf, %lf\n",
             geocode_location_get_description (loc),
             geocode_location_get_latitude (loc),
             geocode_location_get_longitude (loc));
}

static void
print_res (const char *key,
	   const char *value,
	   gpointer    data)
{
	g_print ("\t%s = %s\n", key, value);
}

static void
got_geocode_cb (GObject *source_object,
		GAsyncResult *res,
		gpointer user_data)
{
	GeocodeReverse *object = (GeocodeReverse *) source_object;
	GHashTable *ht;
	GError *error = NULL;

	ht = geocode_reverse_resolve_finish (object, res, &error);
	if (ht == NULL) {
		g_message ("Failed to get geocode: %s", error->message);
		g_error_free (error);
		exit (1);
	}

	g_print ("Got geocode answer:\n");
	g_hash_table_foreach (ht, (GHFunc) print_res, NULL);
	g_hash_table_destroy (ht);

	g_object_unref (object);

	exit (0);
}

static void
got_geocode_search_cb (GObject *source_object,
		       GAsyncResult *res,
		       gpointer user_data)
{
	GeocodeForward *object = (GeocodeForward *) source_object;
	GList *results, *l;
	GError *error = NULL;

	results = geocode_forward_search_finish (object, res, &error);
	if (results == NULL) {
		g_message ("Failed to search geocode: %s", error->message);
		g_error_free (error);
		exit (1);
	}

	for (l = results; l != NULL; l = l->next) {
		GeocodeLocation *loc = l->data;

		g_print ("Got geocode search answer:\n");
		print_loc (loc);
		g_object_unref (loc);
	}
	g_list_free (results);

	g_object_unref (object);

	exit (0);
}

static void
test_rev (void)
{
	GeocodeLocation *loc;
	GeocodeReverse *rev;
	GError *error = NULL;
	GHashTable *ht;

	loc = geocode_location_new (51.237070, -0.589669, GEOCODE_LOCATION_ACCURACY_UNKNOWN);
	rev = geocode_reverse_new_for_location (loc);
	g_object_unref (loc);

	ht = geocode_reverse_resolve (rev, &error);
	if (ht == NULL) {
		g_warning ("Failed at reverse geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);
	g_object_unref (rev);

	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "area"), "Onslow Village") == 0);

	g_print ("Got geocode answer:\n");
	g_hash_table_foreach (ht, (GHFunc) print_res, NULL);
	g_hash_table_destroy (ht);
}

static void
add_attr (GHashTable *ht,
	  const char *key,
	  const char *s)
{
	GValue *value;
	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_static_string (value, s);
	g_hash_table_insert (ht, g_strdup (key), value);
}

static void
test_xep (void)
{
	GHashTable *tp;
	GeocodeForward *object;
	GList *res;
	GeocodeLocation *loc;
	GError *error = NULL;

	tp = g_hash_table_new_full (g_str_hash, g_str_equal,
				    g_free, g_free);
	add_attr (tp, "country", "UK");
	add_attr (tp, "region", "Surrey");
	add_attr (tp, "locality", "Guildford");
	add_attr (tp, "postalcode", "GU2 7");
	add_attr (tp, "street", "Old Palace Rd");
	add_attr (tp, "building", "9");
	add_attr (tp, "description", "My local pub");

	object = geocode_forward_new_for_params (tp);
	g_assert (object != NULL);
	g_hash_table_destroy (tp);

	res = geocode_forward_search (object, &error);
	if (res == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (res != NULL);

	g_object_unref (object);

	loc = res->data;
	g_assert_cmpfloat (geocode_location_get_latitude (loc), ==, -0.589669);
	g_assert_cmpfloat (geocode_location_get_longitude (loc), ==, 51.237070);

	g_object_unref (loc);
	g_list_free (res);
}

static void
test_pub (void)
{
	GeocodeForward *object;
	GError *error = NULL;
	GList *res;
	GeocodeLocation *loc;

	object = geocode_forward_new_for_string ("9, old palace road, guildford, surrey");
	geocode_forward_set_answer_count (object, 1);
	res = geocode_forward_search (object, &error);
	if (res == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (res != NULL);

	g_object_unref (object);

	g_assert_cmpint (g_list_length (res), ==, 1);
	loc = res->data;

	g_assert_cmpfloat (geocode_location_get_latitude (loc), ==, -0.589669);
	g_assert_cmpfloat (geocode_location_get_longitude (loc), ==, 51.237070);

	g_object_unref (loc);
	g_list_free (res);
}

static void
test_search (void)
{
	GeocodeForward *forward;
	GError *error = NULL;
	GList *results, *l;
	gboolean got_france, got_texas;
	char *old_locale;

	old_locale = g_strdup (setlocale(LC_MESSAGES, NULL));
	setlocale (LC_MESSAGES, "en_GB.UTF-8");

	forward = geocode_forward_new_for_string ("paris");
	geocode_forward_set_answer_count (forward, 0);
	results = geocode_forward_search (forward, &error);
	if (results == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (results != NULL);

	g_object_unref (forward);

	g_assert_cmpint (g_list_length (results), ==, 38);

	/* We need to find Paris in France and in Texas */
	got_france = FALSE;
	got_texas = FALSE;
	for (l = results; l != NULL; l = l->next) {
		GeocodeLocation *loc = l->data;

		if (g_strcmp0 (geocode_location_get_description (loc), "Paris, France") == 0)
			got_france = TRUE;
		else if (g_strcmp0 (geocode_location_get_description (loc), "Paris, Texas, United States") == 0)
			got_texas = TRUE;

		g_object_unref (loc);

		if (got_france && got_texas)
			break;
	}
	g_list_free (results);

	g_assert (got_france);
	g_assert (got_texas);

	setlocale (LC_MESSAGES, old_locale);
	g_free (old_locale);
}

static void
test_search_lat_long (void)
{
	GeocodeForward *object;
	GError *error = NULL;
	GList *res;
	GeocodeLocation *loc;

	object = geocode_forward_new_for_string ("Santa María del Río");
	res = geocode_forward_search (object, &error);
	if (res == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (res != NULL);
	g_object_unref (object);

	loc = res->data;
	g_assert_cmpfloat (geocode_location_get_latitude (loc) - 21.800699, <, 0.000001);
	g_assert_cmpfloat (geocode_location_get_longitude (loc) - -100.735626, <, 0.000001);

	g_list_free_full (res, (GDestroyNotify) g_object_unref);
}

/* Test case from:
 * http://andrew.hedges.name/experiments/haversine/ */
static void
test_distance (void)
{
	GeocodeLocation *loca, *locb;

	/* 1600 Pennsylvania Ave NW, Washington, DC */
	loca = geocode_location_new (38.898556, -77.037852, GEOCODE_LOCATION_ACCURACY_UNKNOWN);
	/* 1600 Pennsylvania Ave NW, Washington, DC */
	locb = geocode_location_new (38.897147, -77.043934, GEOCODE_LOCATION_ACCURACY_UNKNOWN);

	g_assert_cmpfloat (geocode_location_get_distance_from (loca, locb) - 0.549311, <, 0.000001);
}

static void
test_locale (void)
{
	GeocodeForward *object;
	GError *error = NULL;
	GList *res;
	GeocodeLocation *loc;
	char *old_locale;

	old_locale = g_strdup (setlocale(LC_MESSAGES, NULL));

	/* Check Moscow's name in Czech */
	setlocale (LC_MESSAGES, "cs_CZ.UTF-8");
	object = geocode_forward_new_for_string ("moscow");
	res = geocode_forward_search (object, &error);
	if (res == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (res != NULL);
	g_object_unref (object);

	loc = res->data;
	g_assert_cmpstr (geocode_location_get_description (loc), ==, "Moskva, Rusko");
	g_assert_cmpfloat (geocode_location_get_latitude (loc) - 55.756950, <, 0.000001);
	g_assert_cmpfloat (geocode_location_get_longitude (loc) - 37.614971, <, 0.000001);
	print_loc (loc);

	g_list_free_full (res, (GDestroyNotify) g_object_unref);

	/* Check Bonneville's region in French */
	setlocale (LC_MESSAGES, "fr_FR.UTF-8");
	object = geocode_forward_new_for_string ("bonneville");
	res = geocode_forward_search (object, &error);
	if (res == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (res != NULL);
	g_object_unref (object);

	loc = res->data;
	g_assert_cmpstr (geocode_location_get_description (loc), ==, "Bonneville, Rhône-Alpes, France");
	print_loc (loc);

	g_list_free_full (res, (GDestroyNotify) g_object_unref);

	/* And reset the locale */
	setlocale (LC_MESSAGES, old_locale);
	g_free (old_locale);
}

static void
test_resolve_json (void)
{
	GHashTable *ht;
	GError *error = NULL;
	guint i;
	struct {
		const char *fname;
		const char *error;
		const char *key;
		const char *value;
	} tests[] = {
		{ "placefinder-area.json", NULL, "area", "Onslow Village" },
		{ "placefinder-got-error.json", "You gotz done!" },
		{ "placefinder-no-results.json", "No matches found for request", NULL, NULL },
	};

	for (i = 0; i < G_N_ELEMENTS (tests); i++) {
		char *contents;
		char *filename;

		filename = g_strdup_printf (TEST_SRCDIR "/%s", tests[i].fname);
		if (g_file_get_contents (filename, &contents, NULL, &error) == FALSE) {
			g_critical ("Couldn't load contents of '%s': %s",
				    filename, error->message);
		}
		g_free (filename);

		ht = _geocode_parse_resolve_json (contents, &error);
		g_free (contents);

		if (tests[i].error) {
			g_assert (ht == NULL);
			g_assert_cmpstr (error->message, ==, tests[i].error);
		} else {
			g_assert (ht != NULL);
		}

		if (ht == NULL) {
			g_error_free (error);
			error = NULL;
			continue;
		}

		g_assert_cmpstr (g_hash_table_lookup (ht, tests[i].key), ==, tests[i].value);
		g_hash_table_destroy (ht);
	}
}

static void
test_search_json (void)
{
	GError *error = NULL;
	GList *list;
	char *contents;
	GeocodeLocation *loc;

	if (g_file_get_contents (TEST_SRCDIR "/geoplanet-rio.json",
				 &contents, NULL, &error) == FALSE) {
		g_critical ("Couldn't load contents of '%s': %s",
			    TEST_SRCDIR "/geoplanet-rio.json", error->message);
	}
	list = _geocode_parse_search_json (contents, &error);

	g_assert (list != NULL);
	g_assert_cmpint (g_list_length (list), ==, 10);

	loc = list->data;
	g_assert_cmpstr (geocode_location_get_description (loc), ==, "Rio de Janeiro, Brazil");

	g_list_free_full (list, (GDestroyNotify) g_object_unref);
}

static GeocodeLocation *
new_loc (void)
{
	gdouble latitude, longitude;

	if (params[0] == NULL ||
	    *params[0] == '\0' ||
	    params[1] == NULL ||
	    *params[1] == '\0')
		return NULL;
	latitude = g_ascii_strtod (params[0], NULL);
	longitude = g_ascii_strtod (params[1], NULL);
	return geocode_location_new (latitude, longitude, GEOCODE_LOCATION_ACCURACY_UNKNOWN);
}

int main (int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	gboolean do_rev_geocoding = FALSE;
	int answer_count = DEFAULT_ANSWER_COUNT;
	const GOptionEntry entries[] = {
		{ "count", 0, 0, G_OPTION_ARG_INT, &answer_count, "Number of answers to get for forward searches", NULL },
		{ "reverse", 0, 0, G_OPTION_ARG_NONE, &do_rev_geocoding, "Whether to do reverse geocoding for the given parameters", NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &params, NULL, "[KEY=VALUE...]" },
		{ NULL }
	};

	setlocale (LC_ALL, "");
	g_type_init ();
	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	/* Parse our own command-line options */
	context = g_option_context_new ("- test parser functions");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("Option parsing failed: %s\n", error->message);
		return 1;
	}

	if (params == NULL) {
		g_test_add_func ("/geocode/resolve_json", test_resolve_json);
		g_test_add_func ("/geocode/search_json", test_search_json);
		g_test_add_func ("/geocode/reverse", test_rev);
		g_test_add_func ("/geocode/pub", test_pub);
		g_test_add_func ("/geocode/xep-0080", test_xep);
		g_test_add_func ("/geocode/locale", test_locale);
		g_test_add_func ("/geocode/search", test_search);
		g_test_add_func ("/geocode/search_lat_long", test_search_lat_long);
		g_test_add_func ("/geocode/distance", test_distance);
		return g_test_run ();
	}

	if (do_rev_geocoding == FALSE) {
		GeocodeForward *forward;

		forward = geocode_forward_new_for_string (params[0]);
		if (answer_count != DEFAULT_ANSWER_COUNT)
			geocode_forward_set_answer_count (forward, answer_count);
		geocode_forward_search_async (forward, NULL, got_geocode_search_cb, NULL);
	} else {
		GeocodeReverse *reverse;
		GeocodeLocation *loc;

		loc = new_loc ();
		if (loc == NULL) {
			g_print ("Options parsing failed: Use for example\n"
				 "test-gcglib --reverse -- 51.237070 -0.589669\n");
			return 1;
		}
		print_loc (loc);
		reverse = geocode_reverse_new_for_location (loc);
		g_object_unref (loc);
		geocode_reverse_resolve_async (reverse, NULL, got_geocode_cb, NULL);
	}

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	return 0;
}

