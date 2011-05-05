
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
	GeocodeObject *object = (GeocodeObject *) source_object;
	GHashTable *ht;
	GError *error = NULL;

	ht = geocode_object_resolve_finish (object, res, &error);
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
test_rev (void)
{
	GeocodeObject *object;
	GError *error = NULL;
	GHashTable *ht;

	object = geocode_object_new_for_coords (51.237070, -0.589669);
	ht = geocode_object_resolve (object, &error);
	if (ht == NULL) {
		g_warning ("Failed at reverse geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);
	g_object_unref (object);

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
	GeocodeObject *object;
	GHashTable *ht;
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

	object = geocode_object_new_for_params (tp);
	g_assert (object != NULL);
	g_hash_table_destroy (tp);

	ht = geocode_object_resolve (object, &error);
	if (ht == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);

	g_object_unref (object);
	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "longitude"), "-0.589669") == 0);
	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "latitude"), "51.237070") == 0);

	g_print ("Got geocode answer:\n");
	g_hash_table_foreach (ht, (GHFunc) print_res, NULL);
	g_hash_table_destroy (ht);
}

static void
test_pub (void)
{
	GeocodeObject *object;
	GError *error = NULL;
	GHashTable *ht;

	object = geocode_object_new ();
	geocode_object_add (object, "location", "9, old palace road, guildford, surrey");
	ht = geocode_object_resolve (object, &error);
	if (ht == NULL) {
		g_warning ("Failed at geocoding: %s", error->message);
		g_error_free (error);
	}
	g_assert (ht != NULL);

	g_object_unref (object);
	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "longitude"), "-0.589669") == 0);
	g_assert (g_strcmp0 (g_hash_table_lookup (ht, "latitude"), "51.237070") == 0);

	g_print ("Got geocode answer:\n");
	g_hash_table_foreach (ht, (GHFunc) print_res, NULL);
	g_hash_table_destroy (ht);
}

static void
test_json (void)
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
		ht = _geocode_parse_json (tests[i].test, &error);

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

int main (int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	GeocodeObject *object;
	const GOptionEntry entries[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &params, NULL, "[KEY=VALUE...]" },
		{ NULL }
	};
	guint i;

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
		g_test_add_func ("/geocode/json", test_json);
		g_test_add_func ("/geocode/reverse", test_rev);
		g_test_add_func ("/geocode/pub", test_pub);
		g_test_add_func ("/geocode/xep-0080", test_xep);
		return g_test_run ();
	}

	object = geocode_object_new ();

	for (i = 0; params[i] != NULL; i++) {
		char **items;

		items = g_strsplit (params[i], "=", 2);
		if (items[0] == NULL || *items[0] == '\0' ||
		    items[1] == NULL || *items[1] == '\0') {
			g_warning ("Failed to parse parameter: '%s'", params[i]);
			return 1;
		}

		geocode_object_add (object, items[0], items[1]);
		g_strfreev (items);
	}
	geocode_object_resolve_async (object, NULL, got_geocode_cb, NULL);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	return 0;
}

