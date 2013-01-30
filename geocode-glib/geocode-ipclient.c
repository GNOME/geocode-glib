/*
   Copyright (C) 2013 Satabdi Das

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

   Authors: Satabdi Das <satabdidas@gmail.com>

 */

#include <stdlib.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "geocode-ipclient.h"

struct _GeocodeIpclientPrivate {
        char *ip;
};

G_DEFINE_TYPE (GeocodeIpclient, geocode_ipclient, G_TYPE_OBJECT)

static void
geocode_ipclient_finalize (GObject *gipclient)
{
        GeocodeIpclient *ipclient = (GeocodeIpclient *) gipclient;

        g_free (ipclient->priv->ip);

        G_OBJECT_CLASS (geocode_ipclient_parent_class)->finalize (gipclient);
}

static void
geocode_ipclient_class_init (GeocodeIpclientClass *klass)
{
        GObjectClass *gipclient_class = G_OBJECT_CLASS (klass);

        gipclient_class->finalize = geocode_ipclient_finalize;

        g_type_class_add_private (klass, sizeof (GeocodeIpclientPrivate));
}

static void
geocode_ipclient_init (GeocodeIpclient *ipclient)
{
        ipclient->priv = G_TYPE_INSTANCE_GET_PRIVATE ((ipclient), GEOCODE_TYPE_IPCLIENT, GeocodeIpclientPrivate);
}

GeocodeIpclient *
geocode_ipclient_new_for_ip (const char *ip)
{
        GeocodeIpclient *ipclient;

        ipclient = g_object_new (GEOCODE_TYPE_IPCLIENT, NULL);
        ipclient->priv->ip = g_strdup (ip);

        return ipclient;
}

GeocodeIpclient *
geocode_ipclient_new (void)
{
        return geocode_ipclient_new_for_ip (NULL);
}

static GFile *
get_search_query (GeocodeIpclient *ipclient)
{
        GFile * ret;
        GHashTable *ht;
        char *query_string;
        char *uri;
        const char *ipaddress;
        /* FIXME: the server uri needs to be made a property */
        const char *server_uri = "http://127.0.0.1:12345/cgi-bin/geoip-lookup";

        ipaddress = ipclient->priv->ip;
        if (ipaddress) {
                ht = g_hash_table_new (g_str_hash, g_str_equal);
                g_hash_table_insert (ht, "ip", g_strdup (ipaddress));
                query_string = soup_form_encode_hash (ht);
                g_hash_table_destroy (ht);

                uri = g_strdup_printf ("%s?%s", server_uri, query_string);
                g_free (query_string);
        } else
                uri = g_strdup (server_uri);

        ret = g_file_new_for_uri (uri);
        g_free (uri);

        return ret;
}

static void
query_callback (GObject        *source_forward,
                GAsyncResult   *res,
                gpointer        user_data)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
        GFile *query;
        GError *error = NULL;
        char *contents;

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
        if (contents == NULL) {
                g_simple_async_result_set_from_error (simple, error);
                g_simple_async_result_complete_in_idle (simple);
                g_object_unref (simple);
                g_error_free (error);
                return;
        }
        g_simple_async_result_set_op_res_gpointer (simple, contents, NULL);
        g_simple_async_result_complete_in_idle (simple);
        g_object_unref (simple);
}

void
geocode_ipclient_search_async (GeocodeIpclient           *ipclient,
                               GCancellable              *cancellable,
                               GAsyncReadyCallback       callback,
                               gpointer                  user_data)
{
        GSimpleAsyncResult *simple;
        GFile *query;
        GError *error = NULL;

        g_return_if_fail (GEOCODE_IS_IPCLIENT (ipclient));

        simple = g_simple_async_result_new (G_OBJECT (ipclient),
                                            callback,
                                            user_data,
                                            geocode_ipclient_search_async);

        query = get_search_query (ipclient);
        if (!query) {
                g_simple_async_result_take_error (simple, error);
                g_simple_async_result_complete_in_idle (simple);
                g_object_unref (simple);
                return;
        }
        g_file_load_contents_async (query,
                                    cancellable,
                                    query_callback,
                                    simple);
        g_object_unref (query);
}

char *
geocode_ipclient_search_finish (GeocodeIpclient *ipclient,
                                GAsyncResult    *res,
                                GError          **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
        char *contents = NULL;

        g_return_val_if_fail (GEOCODE_IS_IPCLIENT (ipclient), NULL);

        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_ipclient_search_async);

        if (g_simple_async_result_propagate_error (simple, error))
                return NULL;

        contents = g_simple_async_result_get_op_res_gpointer (simple);
        return contents;
}

char *
geocode_ipclient_search (GeocodeIpclient        *ipclient,
                         GError                 **error)
{
        char *contents;
        GFile *query;

        g_return_val_if_fail (GEOCODE_IS_IPCLIENT (ipclient), NULL);

        query = get_search_query (ipclient);

        if (!query)
                return NULL;
        if (g_file_load_contents (query,
                                  NULL,
                                  &contents,
                                  NULL,
                                  NULL,
                                  error) == FALSE) {
                g_object_unref (query);
                return NULL;
        }
        g_object_unref (query);

        return contents;
}
