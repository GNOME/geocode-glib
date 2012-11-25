
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
	g_print ("\t%s @ %lf, %lf\n", loc->description, loc->latitude, loc->longitude);
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
		geocode_location_free (loc);
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

	loc = geocode_location_new (51.237070, -0.589669);
	rev = geocode_reverse_new_for_location (loc);
	geocode_location_free (loc);

	ht = geocode_reverse_resolve (rev, &error);
	if (ht == NULL) {
		g_warning ("Failed at reverse geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);
	g_object_unref (rev);

	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "neighborhood"), "Onslow Village") == 0);

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
	g_assert_cmpfloat (loc->latitude, ==, -0.589669);
	g_assert_cmpfloat (loc->longitude, ==, 51.237070);

	geocode_location_free (loc);
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

	g_assert_cmpfloat (loc->latitude, ==, -0.589669);
	g_assert_cmpfloat (loc->longitude, ==, 51.237070);

	geocode_location_free (loc);
	g_list_free (res);
}

static void
test_search (void)
{
	GeocodeForward *forward;
	GError *error = NULL;
	GList *results, *l;
	gboolean got_france, got_texas;

	forward = geocode_forward_new_for_string ("paris");
	results = geocode_forward_search (forward, &error);
	if (results == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (results != NULL);

	g_object_unref (forward);

	/* We need to find Paris in France and in Texas */
	got_france = FALSE;
	got_texas = FALSE;
	for (l = results; l != NULL; l = l->next) {
		GeocodeLocation *loc = l->data;

		/* FIXME: implement once description
		 * is generated properly */
#if 0
		if (key_is_value (ht, "country", "France") &&
		    key_is_value (ht, "name", "Paris"))
			got_france = TRUE;
		else if (key_is_value (ht, "admin1", "Texas") &&
			 key_is_value (ht, "name", "Paris"))
			got_texas = TRUE;
		g_hash_table_destroy (ht);
#endif
		geocode_location_free (loc);

		if (got_france && got_texas)
			break;
	}
	g_list_free (results);

	/* FIXME
	g_assert (got_france);
	g_assert (got_texas); */
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
	g_assert_cmpstr (loc->description, ==, "Moskva");
	g_assert_cmpfloat (loc->latitude - 37.614971, <, 0.000001);
	g_assert_cmpfloat (loc->longitude - 55.756950, <, 0.000001);

	g_list_free_full (res, (GDestroyNotify) geocode_location_free);

	/* FIXME, when we have a better way to
	 * differentiate the results */
#if 0
	/* Check Lyon's region in French */
	setlocale (LC_MESSAGES, "fr_FR.UTF-8");
	object = geocode_object_new_for_location ("lyon");
	ht = geocode_object_resolve (object, &error);
	if (ht == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);
	g_object_unref (object);
	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "state"), "Rhône-Alpes") == 0);
	g_print ("Got geocode answer:\n");
	g_hash_table_foreach (ht, (GHFunc) print_res, NULL);
	g_hash_table_destroy (ht);
#endif
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
		const char *test;
		const char *error;
		const char *key;
		const char *value;
	} tests[] = {
		{ "{\"ResultSet\":{\"version\":\"1.0\",\"Error\":0,\"ErrorMessage\":\"No error\",\"Locale\":\"us_US\",\"Quality\":99,\"Found\":1,\"Results\":[{\"quality\":99,\"latitude\":\"51.237100\",\"longitude\":\"-0.589669\",\"offsetlat\":\"51.237100\",\"offsetlon\":\"-0.589669\",\"radius\":500,\"name\":\"51.2371, -0.589669\",\"line1\":\"9 Old Palace Road\",\"line2\":\"Guildford\",\"line3\":\"GU2 7\",\"line4\":\"United Kingdom\",\"house\":\"9\",\"street\":\"Old Palace Road\",\"xstreet\":\"\",\"unittype\":\"\",\"unit\":\"\",\"postal\":\"GU2 7\",\"neighborhood\":\"Onslow Village\",\"city\":\"Guildford\",\"county\":\"Surrey\",\"state\":\"England\",\"country\":\"United Kingdom\",\"countrycode\":\"GB\",\"statecode\":\"ENG\",\"countycode\":\"SRY\",\"timezone\":\"Europe\\/London\",\"hash\":\"\",\"woeid\":26347368,\"woetype\":11,\"uzip\":\"GU2 7\",\"airport\":\"LHR\"}]}}", NULL, "neighborhood", "Onslow Village" },
		{ "{\"ResultSet\":{\"version\":\"1.0\",\"Error\":107,\"ErrorMessage\":\"You gotz done!\",\"Locale\":\"us_US\",\"Quality\":99,\"Found\":1,\"Results\":[{\"quality\":99,\"latitude\":\"51.237100\",\"longitude\":\"-0.589669\",\"offsetlat\":\"51.237100\",\"offsetlon\":\"-0.589669\",\"radius\":500,\"name\":\"51.2371, -0.589669\",\"line1\":\"9 Old Palace Road\",\"line2\":\"Guildford\",\"line3\":\"GU2 7\",\"line4\":\"United Kingdom\",\"house\":\"9\",\"street\":\"Old Palace Road\",\"xstreet\":\"\",\"unittype\":\"\",\"unit\":\"\",\"postal\":\"GU2 7\",\"neighborhood\":\"Onslow Village\",\"city\":\"Guildford\",\"county\":\"Surrey\",\"state\":\"England\",\"country\":\"United Kingdom\",\"countrycode\":\"GB\",\"statecode\":\"ENG\",\"countycode\":\"SRY\",\"timezone\":\"Europe\\/London\",\"hash\":\"\",\"woeid\":26347368,\"woetype\":11,\"uzip\":\"GU2 7\",\"airport\":\"LHR\"}]}}", "You gotz done!" },
		{ "{\"ResultSet\":{\"version\":\"1.0\",\"Error\":0,\"ErrorMessage\":\"No error\",\"Locale\":\"us_US\",\"Quality\":10,\"Found\":0}}", "No matches found for request", NULL, NULL },
	};

	for (i = 0; i < G_N_ELEMENTS (tests); i++) {
		ht = _geocode_parse_resolve_json (tests[i].test, &error);

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
	/* FIXME: Implement */
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
	return geocode_location_new (latitude, longitude);
}

int main (int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	gboolean do_rev_geocoding = FALSE;
	int answer_count = 10;
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
		return g_test_run ();
	}

	if (do_rev_geocoding == FALSE) {
		GeocodeForward *forward;

		forward = geocode_forward_new_for_string (params[0]);
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
		geocode_location_free (loc);
		geocode_reverse_resolve_async (reverse, NULL, got_geocode_cb, NULL);
	}

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	return 0;
}

