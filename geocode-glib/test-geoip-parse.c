#include "config.h"
#include <locale.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include "geocode-glib-private.h"

static void
test_parse_json (gconstpointer data)
{
        GeocodeLocation *location;
        GFile *file;
        char *contents;
        char *path;
        GError *error = NULL;

        path = g_build_filename(TEST_SRCDIR, (const char *) data, NULL);
        g_assert (path != NULL);
        file = g_file_new_for_path(path);
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
        g_free(contents);
        g_free(path);
        geocode_location_free (location);
}

int main (int argc, char **argv)
{
        setlocale (LC_ALL, "");
        g_type_init ();
        g_test_init (&argc, &argv, NULL);
        g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

        g_test_add_data_func ("/geoip/parse-freegeoip-response",
                              "freegeoip-results.json",
                              test_parse_json);

        g_test_add_data_func ("/geoip/parse-geocode-glib-response",
                              "gglib-ip-server-results.json",
                              test_parse_json);

        return g_test_run ();
}
