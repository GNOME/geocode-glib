
#include "config.h"
#include <locale.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <geocode-glib/geocode-ipclient.h>
#include <geocode-glib/geocode-glib-private.h>

static GMainLoop *loop = NULL;

typedef struct {
        const char *ip;

        double expected_latitude;
        double expected_longitude;
        const char *expected_description;
} TestData;

static void
test_parse_json (gconstpointer data)
{
        GeocodeLocation *location;
        GFile *file;
        char *contents;
        char *path;
        GError *error = NULL;

        path = g_build_filename (TEST_SRCDIR, (const char *) data, NULL);
        g_assert (path != NULL);
        file = g_file_new_for_path (path);
        if (!g_file_load_contents (file, NULL, &contents, NULL, NULL, &error)) {
                g_warning ("Failed to load file '%s': %s", path, error->message);
                g_error_free (error);
                g_assert_not_reached ();
        }

        location = _geocode_ip_json_to_location (contents, &error);
        if (!location) {
                g_warning ("Failed to parse '%s': %s", path, error->message);
                g_error_free (error);
        }
        g_free (contents);
        g_free (path);
        g_object_unref (location);
}


static void
test_search (gconstpointer data)
{
        GeocodeIpclient *ipclient;
        GError *error = NULL;
        GeocodeLocation *location;
        TestData *test_data = (TestData *) data;

        if (test_data->ip)
                ipclient = geocode_ipclient_new_for_ip (test_data->ip);
        else {
                ipclient = geocode_ipclient_new ();
        }
        location = geocode_ipclient_search (ipclient, &error);
        if (!location) {
                g_warning ("Failed at getting the geolocation information: %s", error->message);
                g_error_free (error);
        }
        g_assert (location != NULL);
        g_assert (geocode_location_get_latitude (location) == test_data->expected_latitude);
        g_assert (geocode_location_get_longitude (location) == test_data->expected_longitude);
        g_assert (geocode_location_get_description (location) != NULL);
        g_assert (strcmp (geocode_location_get_description (location),
                          test_data->expected_description) == 0);
        g_assert (geocode_location_get_accuracy (location) != GEOCODE_LOCATION_ACCURACY_UNKNOWN);
        g_object_unref (location);
        g_object_unref (ipclient);
}

static void
print_geolocation_info_cb (GObject          *source_object,
                           GAsyncResult     *res,
                           gpointer         user_data)
{
        GeocodeIpclient *object = (GeocodeIpclient *) source_object;
        GError *error = NULL;
        GeocodeLocation *location;

        location = geocode_ipclient_search_finish (object, res, &error);
        if (location == NULL) {
                g_message ("Failed to search the geolocation info: %s", error->message);
                g_error_free (error);
                exit (1);
        }
        g_print ("Location: %s (%f,%f)\n",
                 geocode_location_get_description (location),
                 geocode_location_get_latitude (location),
                 geocode_location_get_longitude (location));

        g_object_unref (location);
        g_object_unref (object);
        exit (0);
}

int main (int argc, char **argv)
{
        GError *error = NULL;
        GOptionContext *context;
        GeocodeIpclient *ipclient;
        TestData data = { NULL, 0.0f, 0.0f, NULL };
        gboolean regr = FALSE;
        const GOptionEntry entries[] = {
                { "ip", 0, 0, G_OPTION_ARG_STRING, &data.ip, "The ip address for which to search the geolocation data", NULL },
                { "regr", 0, 0, G_OPTION_ARG_NONE, &regr, "Run the default testcases", NULL },
                { NULL }
        };

        setlocale (LC_ALL, "");
#if (!GLIB_CHECK_VERSION (2, 36, 0))
        g_type_init ();
#endif
        g_test_init (&argc, &argv, NULL);
        g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

        /* Parse our own command-line options */
        context = g_option_context_new ("- test parser functions");
        g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

        if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
                g_print ("Option parsing failed: %s\n", error->message);
                return 1;
        }

        if (regr == TRUE) {
                data.expected_latitude = 43.089199f;
                data.expected_longitude = -76.025002f;
                data.expected_description = "East Syracuse, New York, United States";

                g_test_add_data_func ("/geocode/search", &data, test_search);

                data.ip = "24.24.24.24";
                g_test_add_data_func ("/geoip/search_with_ip", &data, test_search);

                g_test_add_data_func ("/geoip/parse-freegeoip-response",
                                      "freegeoip-results.json",
                                      test_parse_json);

                g_test_add_data_func ("/geoip/parse-geocode-glib-response",
                                      "gglib-ip-server-results.json",
                                      test_parse_json);
                return g_test_run ();
        }

        if (data.ip)
                ipclient = geocode_ipclient_new_for_ip (data.ip);
        else
                ipclient = geocode_ipclient_new ();
        geocode_ipclient_search_async (ipclient, NULL, print_geolocation_info_cb, NULL);

        loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (loop);

        return 0;
}
