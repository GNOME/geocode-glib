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
#include <json-glib/json-glib.h>
#include <string.h>
#include <geocode-glib/geocode-glib-private.h>
#include <geocode-glib/geocode-ipclient.h>
#include <geocode-glib/geocode-error.h>
#include <geocode-glib/geocode-enum-types.h>
#include <geocode-glib/geocode-ip-server/geoip-server.h>

/**
 * SECTION:geocode-ipclient
 * @short_description: GeoIP client
 * @include: geocode-glib/geocode-ipclient.h
 *
 * Contains functions to get the geolocation corresponding to IP addresses from a server.
 **/

enum {
        PROP_0,
        PROP_SERVER,
        PROP_COMPAT_MODE,
        N_PROPERTIES
};

struct _GeocodeIpclientPrivate {
        SoupSession *soup_session;

        char *ip;
        char *server;
        gboolean compat_mode;

};

G_DEFINE_TYPE (GeocodeIpclient, geocode_ipclient, G_TYPE_OBJECT)

static void
geocode_ipclient_set_server (GeocodeIpclient *ipclient,
                             const char      *server)
{
        g_return_if_fail (server != NULL);
        g_return_if_fail (g_str_has_prefix (server, "http://") ||
                          g_str_has_prefix (server, "https://"));

        g_free (ipclient->priv->server);
        ipclient->priv->server = g_strdup (server);

}

static void
geocode_ipclient_set_property (GObject           *gipclient,
                               guint              property_id,
                               const GValue      *value,
                               GParamSpec        *pspec)
{
        GeocodeIpclient *ipclient = (GeocodeIpclient *) gipclient;

        switch (property_id) {
                case PROP_SERVER:
                        geocode_ipclient_set_server (ipclient,
                                                     g_value_get_string (value));
                        break;
                case PROP_COMPAT_MODE:
                        ipclient->priv->compat_mode = g_value_get_boolean (value);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (gipclient,
                                                           property_id,
                                                           pspec);
                        break;
        }
}

static void
geocode_ipclient_get_property (GObject           *gipclient,
                               guint              property_id,
                               GValue            *value,
                               GParamSpec        *pspec)
{
        GeocodeIpclient *ipclient = (GeocodeIpclient *) gipclient;
        switch (property_id) {
                case PROP_SERVER:
                        g_value_set_string (value, ipclient->priv->server);
                        break;

                case PROP_COMPAT_MODE:
                        g_value_set_boolean (value, ipclient->priv->compat_mode);
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (gipclient,
                                                           property_id,
                                                           pspec);
                        break;
        }
}

static void
geocode_ipclient_finalize (GObject *gipclient)
{
        GeocodeIpclient *ipclient = (GeocodeIpclient *) gipclient;

        g_clear_object (&ipclient->priv->soup_session);
        g_free (ipclient->priv->ip);
        g_free (ipclient->priv->server);

        G_OBJECT_CLASS (geocode_ipclient_parent_class)->finalize (gipclient);
}

static void
geocode_ipclient_class_init (GeocodeIpclientClass *klass)
{
        GObjectClass *gipclient_class = G_OBJECT_CLASS (klass);

        gipclient_class->finalize = geocode_ipclient_finalize;
        gipclient_class->set_property = geocode_ipclient_set_property;
        gipclient_class->get_property = geocode_ipclient_get_property;

        g_type_class_add_private (klass, sizeof (GeocodeIpclientPrivate));

        g_object_class_install_property (gipclient_class,
                                         PROP_SERVER,
                                         g_param_spec_string ("server",
                                                              "server uri",
                                                              "server uri",
                                                              "http://127.0.0.1:12345/cgi-bin/geoip-lookup",
                                                              G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

        /**
         * GeocodeIpclient:compatibility-mode:
         *
         * Enable this mode if you are using freegeoip's (or a compatible) service.
         */
        g_object_class_install_property (gipclient_class,
                                         PROP_COMPAT_MODE,
                                         g_param_spec_boolean ("compatibility-mode",
                                                               "compatiblity mode",
                                                               "compatiblity Mode",
                                                               FALSE,
                                                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
geocode_ipclient_init (GeocodeIpclient *ipclient)
{
        ipclient->priv = G_TYPE_INSTANCE_GET_PRIVATE ((ipclient), GEOCODE_TYPE_IPCLIENT, GeocodeIpclientPrivate);

        ipclient->priv->soup_session = soup_session_new ();
}

/**
 * geocode_ipclient_new_for_ip:
 * @str: The IP address
 *
 * Creates a new #GeocodeIpclient to fetch the geolocation data
 * Use geocode_ipclient_search_async() to query the server
 *
 * Returns: a new #GeocodeIpclient. Use g_object_unref() when done.
 **/
GeocodeIpclient *
geocode_ipclient_new_for_ip (const char *ip)
{
        GeocodeIpclient *ipclient;

        ipclient = g_object_new (GEOCODE_TYPE_IPCLIENT, NULL);
        ipclient->priv->ip = g_strdup (ip);

        return ipclient;
}

/**
 * geocode_ipclient_new:
 *
 * Creates a new #GeocodeIpclient to fetch the geolocation data.
 * Here the IP address is not provided the by client, hence the server
 * will try to get the IP address from various proxy variables.
 * Use geocode_ipclient_search_async() to query the server
 *
 * Returns: a new #GeocodeIpclient. Use g_object_unref() when done.
 **/
GeocodeIpclient *
geocode_ipclient_new (void)
{
        return geocode_ipclient_new_for_ip (NULL);
}

static SoupMessage *
get_search_query (GeocodeIpclient *ipclient)
{
        SoupMessage *ret;
        GHashTable *ht;
        char *query_string;
        char *uri;
        const char *ipaddress;

        ipaddress = ipclient->priv->ip;
        if (ipaddress) {
                if (ipclient->priv->compat_mode) {
                        char *escaped;

                        escaped = soup_uri_encode (ipaddress, NULL);
                        uri = g_strdup_printf ("%s/%s",
                                               ipclient->priv->server,
                                               escaped);
                        g_free (escaped);
                } else {
                        ht = g_hash_table_new (g_str_hash, g_str_equal);
                        g_hash_table_insert (ht, (gpointer) "ip", g_strdup (ipaddress));
                        query_string = soup_form_encode_hash (ht);
                        g_hash_table_destroy (ht);

                        uri = g_strdup_printf ("%s?%s",
                                               ipclient->priv->server,
                                               query_string);
                        g_free (query_string);
                }
        } else
                uri = g_strdup (ipclient->priv->server);

        ret = soup_message_new ("GET", uri);
        g_free (uri);

        return ret;
}

static void
query_callback (SoupSession *session,
                SoupMessage *query,
                gpointer     user_data)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
        GError *error = NULL;
        char *contents;

        if (query->status_code != SOUP_STATUS_OK) {
		g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     query->reason_phrase ? query->reason_phrase : "Query failed");
                g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		return;
	}

        contents = g_strndup (query->response_body->data, query->response_body->length);
        g_simple_async_result_set_op_res_gpointer (simple, contents, NULL);

        g_simple_async_result_complete_in_idle (simple);
        g_object_unref (simple);
}

/**
 * geocode_ipclient_search_async:
 * @ipclient: a #GeocodeIpclient representing a query
 * @cancellable: optional #GCancellable forward, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to callback function
 *
 * Asynchronously performs a query to get the geolocation information
 * from the server. Use geocode_ipclient_search() to do the same
 * thing synchronously.
 *
 * When the operation is finished, @callback will be called. You can then call
 * geocode_ipclient_search_finish() to get the result of the operation.
 **/
void
geocode_ipclient_search_async (GeocodeIpclient    *ipclient,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
        GSimpleAsyncResult *simple;
        SoupMessage *query;

        g_return_if_fail (GEOCODE_IS_IPCLIENT (ipclient));
        g_return_if_fail (ipclient->priv->server != NULL);

        simple = g_simple_async_result_new (G_OBJECT (ipclient),
                                            callback,
                                            user_data,
                                            geocode_ipclient_search_async);

        query = get_search_query (ipclient);
        soup_session_queue_message (ipclient->priv->soup_session,
                                    query,
                                    query_callback,
                                    simple);
}

static gboolean
parse_server_error (JsonObject *object, GError **error)
{
        GeoipServerError server_error_code;
        int error_code;
        const char *error_message;

        if (!json_object_has_member (object, "error_code"))
            return FALSE;

        server_error_code = json_object_get_int_member (object, "error_code");
        switch (server_error_code) {
                case INVALID_IP_ADDRESS_ERR:
                        error_code = GEOCODE_ERROR_INVALID_ARGUMENTS;
                        break;
                case INVALID_ENTRY_ERR:
                        error_code = GEOCODE_ERROR_NO_MATCHES;
                        break;
                case DATABASE_ERR:
                        error_code = GEOCODE_ERROR_INTERNAL_SERVER;
                        break;
                default:
                        g_assert_not_reached ();
        }

        error_message = json_object_get_string_member (object, "error_message");

        g_set_error_literal (error,
                             GEOCODE_ERROR,
                             error_code,
                             error_message);

        return TRUE;
}

static gdouble
get_accuracy_from_string (const char *str)
{
        if (strcmp (str, "street") == 0)
                return GEOCODE_LOCATION_ACCURACY_STREET;
        else if (strcmp (str, "city") == 0)
                return GEOCODE_LOCATION_ACCURACY_CITY;
        else if (strcmp (str, "region") == 0)
                return GEOCODE_LOCATION_ACCURACY_REGION;
        else if (strcmp (str, "country") == 0)
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        else if (strcmp (str, "continent") == 0)
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        else
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
}

static gdouble
get_accuracy_from_json_location (JsonObject *object)
{
        if (json_object_has_member (object, "accuracy")) {
                const char *str;

                str = json_object_get_string_member (object, "accuracy");
                return get_accuracy_from_string (str);
        } else if (json_object_has_member (object, "street")) {
                return GEOCODE_LOCATION_ACCURACY_STREET;
        } else if (json_object_has_member (object, "city")) {
                return GEOCODE_LOCATION_ACCURACY_CITY;
        } else if (json_object_has_member (object, "region_name")) {
                return GEOCODE_LOCATION_ACCURACY_REGION;
        } else if (json_object_has_member (object, "country_name")) {
                return GEOCODE_LOCATION_ACCURACY_COUNTRY;
        } else if (json_object_has_member (object, "continent")) {
                return GEOCODE_LOCATION_ACCURACY_CONTINENT;
        } else {
                return GEOCODE_LOCATION_ACCURACY_UNKNOWN;
        }
}

GeocodeLocation *
_geocode_ip_json_to_location (const char *json,
                              GError    **error)
{
        JsonParser *parser;
        JsonNode *node;
        JsonObject *object;
        GeocodeLocation *location;
        gdouble latitude, longitude, accuracy;
        char *desc = NULL;

        parser = json_parser_new ();

        if (!json_parser_load_from_data (parser, json, -1, error))
                return NULL;

        node = json_parser_get_root (parser);
        object = json_node_get_object (node);

        if (parse_server_error (object, error))
                return NULL;

        latitude = json_object_get_double_member (object, "latitude");
        longitude = json_object_get_double_member (object, "longitude");
        accuracy = get_accuracy_from_json_location (object);

        location = geocode_location_new (latitude, longitude, accuracy);

        if (json_object_has_member (object, "country_name")) {
                if (json_object_has_member (object, "region_name")) {
                        if (json_object_has_member (object, "city")) {
                                desc = g_strdup_printf ("%s, %s, %s",
                                                        json_object_get_string_member (object, "city"),
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        } else {
                                desc = g_strdup_printf ("%s, %s",
                                                        json_object_get_string_member (object, "region_name"),
                                                        json_object_get_string_member (object, "country_name"));
                        }
                } else {
                        desc = g_strdup_printf ("%s",
                                                json_object_get_string_member (object, "country_name"));
                }
        }

        if (desc != NULL) {
                geocode_location_set_description (location, desc);
                g_free (desc);
        }

        g_object_unref (parser);

        return location;
}

/**
 * geocode_ipclient_search_finish:
 * @ipclient: a #GeocodeIpclient representing a query
 * @res: a #GAsyncResult
 * @error: a #GError
 *
 * Finishes a geolocation search operation. See geocode_ipclient_search_async().
 *
 * Returns: (transfer full): A #GeocodeLocation object or %NULL in case of
 * errors. Free the returned object with g_object_unref() when done.
 **/
GeocodeLocation *
geocode_ipclient_search_finish (GeocodeIpclient *ipclient,
                                GAsyncResult    *res,
                                GError         **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
        char *contents = NULL;
        GeocodeLocation *location;

        g_return_val_if_fail (GEOCODE_IS_IPCLIENT (ipclient), NULL);

        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == geocode_ipclient_search_async);

        if (g_simple_async_result_propagate_error (simple, error))
                return NULL;

        contents = g_simple_async_result_get_op_res_gpointer (simple);
        location = _geocode_ip_json_to_location (contents, error);
        g_free (contents);

        return location;
}

/**
 * geocode_ipclient_search:
 * @ipclient: a #GeocodeIpclient representing a query
 * @error: a #GError
 *
 * Gets the geolocation data for an IP address from the server.
 *
 * Returns: (transfer full): A #GeocodeLocation object or %NULL in case of
 * errors. Free the returned object with g_object_unref() when done.
 **/
GeocodeLocation *
geocode_ipclient_search (GeocodeIpclient *ipclient,
                         GError         **error)
{
        char *contents;
        SoupMessage *query;
        GeocodeLocation *location;

        g_return_val_if_fail (GEOCODE_IS_IPCLIENT (ipclient), NULL);
        g_return_val_if_fail (ipclient->priv->server != NULL, NULL);

        query = get_search_query (ipclient);

        if (soup_session_send_message (ipclient->priv->soup_session,
                                       query) != SOUP_STATUS_OK) {
                g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     query->reason_phrase ? query->reason_phrase : "Query failed");
                g_object_unref (query);
                return NULL;
        }

        contents = g_strndup (query->response_body->data, query->response_body->length);
        g_object_unref (query);

        location = _geocode_ip_json_to_location (contents, error);
        g_free (contents);

        return location;
}
