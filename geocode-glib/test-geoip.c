
#include "config.h"
#include <locale.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <geocode-ipclient.h>

static GMainLoop *loop = NULL;

static void
test_search (gconstpointer data)
{
        GeocodeIpclient *ipclient;
        GError *error = NULL;
        char *contents;
        const char *ip;

        ip = (const char *) data;
        if (ip)
                ipclient = geocode_ipclient_new_for_ip (ip);
        else
                ipclient = geocode_ipclient_new ();

        contents = geocode_ipclient_search (ipclient, &error);
        if (!contents) {
                g_warning ("Failed at getting the geolocation information: %s", error->message);
                g_error_free (error);
        }
        g_assert (contents != NULL);
        g_assert (strstr (contents, "\"ERROR\"") == NULL);
        g_object_unref (ipclient);
        g_print ("%s\n", contents);
        g_free (contents);
}

static void
print_geolocation_info_cb (GObject          *source_object,
                           GAsyncResult     *res,
                           gpointer         user_data)
{
        GeocodeIpclient *object = (GeocodeIpclient *) source_object;
        GError *error = NULL;
        char *results;

        results = geocode_ipclient_search_finish (object, res, &error);
        if (results == NULL) {
                g_message ("Failed to search the geolocation info: %s", error->message);
                g_error_free (error);
                exit (1);
        }
        g_print ("%s\n", results);
        g_free (results);

        g_object_unref (object);
        exit (0);
}


int main (int argc, char **argv)
{
        GError *error = NULL;
        GOptionContext *context;
        GeocodeIpclient *ipclient;
        char *ip = NULL;
        gboolean regr = FALSE;
        const GOptionEntry entries[] = {
                { "ip", 0, 0, G_OPTION_ARG_STRING, &ip, "The ip address for which to search the geolocation data", NULL },
                { "regr", 0, 0, G_OPTION_ARG_NONE, &regr, "Run the default testcases", NULL },
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

        if (regr == TRUE) {
                g_test_add_data_func ("/geoip/search_with_ip", "24.24.24.24", test_search);
                g_test_add_data_func ("/geocode/search", NULL, test_search);
                return g_test_run ();
        }

        if (ip)
                ipclient = geocode_ipclient_new_for_ip (ip);
        else
                ipclient = geocode_ipclient_new ();
        geocode_ipclient_search_async (ipclient, NULL, print_geolocation_info_cb, NULL);

        loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (loop);

        return 0;
}
