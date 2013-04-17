/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <glib.h>
#include <string.h>
#include <json-glib/json-glib.h>

static void
test_response_data (gconstpointer data)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object;
        double latitude, longitude;

        parser = json_parser_new ();

        g_assert (json_parser_load_from_data (parser, (const char *) data, -1, NULL));

        node = json_parser_get_root (parser);
        g_assert (JSON_NODE_HOLDS_OBJECT (node));
        object = json_node_get_object (node);
        g_assert (object != NULL);

        g_assert (json_object_has_member (object, "ip"));
        g_assert (strcmp (json_object_get_string_member (object, "ip"), "213.243.180.91") == 0);

        g_assert (json_object_has_member (object, "latitude"));
        latitude = json_object_get_double_member (object, "latitude");
        g_assert (latitude > 60.1755 && latitude <= 60.1756);
        g_assert (json_object_has_member (object, "longitude"));
        longitude = json_object_get_double_member (object, "longitude");
        g_assert (longitude >= 24.9342 && longitude < 24.9343);

        g_assert (json_object_has_member (object, "city"));
        g_assert (strcmp (json_object_get_string_member (object, "city"), "Helsinki") == 0);
        g_assert (json_object_has_member (object, "region_name"));
        g_assert (strcmp (json_object_get_string_member (object, "region_name"), "Southern Finland") == 0);
        g_assert (json_object_has_member (object, "country_name"));
        g_assert (strcmp (json_object_get_string_member (object, "country_name"), "Finland") == 0);

        if (json_object_has_member (object, "accuracy"))
            g_assert (strcmp (json_object_get_string_member (object, "accuracy"), "city") == 0);
        if (json_object_has_member (object, "timezone"))
            g_assert (strcmp (json_object_get_string_member (object, "timezone"), "Europe/Helsinki") == 0);

        g_object_unref (parser);
}

static char *
get_freegeoip_response (void)
{
        GFile *file;
        char *contents = NULL;
        char *path;
        GError *error = NULL;

        path = g_build_filename(TEST_SRCDIR, "freegeoip-results.json", NULL);
        g_assert (path != NULL);
        file = g_file_new_for_path(path);
        if (!g_file_load_contents (file, NULL, &contents, NULL, NULL, &error)) {
                g_warning ("Failed to load file '%s': %s", path, error->message);
                g_error_free (error);
                g_assert_not_reached ();
        }

        g_free (path);

        return contents;
}

static char *
get_our_server_response (const char *query)
{
        char *response;
        char *p;
        char *standard_output;
        int exit_status;
        char **environ;
        char *argv[] = { NULL, NULL };
        GError *error = NULL;

        environ = g_get_environ ();
        environ = g_environ_setenv (environ, "QUERY_STRING", query, TRUE);

        argv[0] = g_build_filename (BUILDDIR, "geoip-lookup", NULL);
        g_assert (argv[0] != NULL);

        if (!g_spawn_sync (BUILDDIR,
                           argv,
                           environ,
                           G_SPAWN_STDERR_TO_DEV_NULL,
                           NULL,
                           NULL,
                           &standard_output,
                           NULL,
                           &exit_status,
                           &error)) {
                g_warning ("Failed to execute '%s': %s", argv[0], error->message);
                g_error_free (error);
                g_assert_not_reached ();
        }

        g_strfreev (environ);
        g_free (argv[0]);

        /* Get rid of headers */
        p = strstr (standard_output, "\n\n");
        g_assert (p != NULL);
        p += 2;

        response = g_strdup (p);
        g_free (standard_output);

        return response;
}

int
main (int argc, char **argv)
{
        char *our_response, *freegeoip_response;
        int ret;

#if (!GLIB_CHECK_VERSION (2, 36, 0))
        g_type_init ();
#endif
        g_test_init (&argc, &argv, NULL);
        g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

        our_response = get_our_server_response ("ip=213.243.180.91");
        g_test_add_data_func ("/geoip/geocode-glib-server-format",
                              our_response,
                              test_response_data);

        freegeoip_response = get_freegeoip_response ();
        g_test_add_data_func ("/geoip/geocode-freegeoip-format",
                              freegeoip_response,
                              test_response_data);

        ret = g_test_run ();
        g_free (freegeoip_response);
        g_free (our_response);

        return ret;
}
