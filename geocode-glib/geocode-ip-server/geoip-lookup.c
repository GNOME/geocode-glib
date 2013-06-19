/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <GeoIP.h>
#include <GeoIPCity.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "geoip-server.h"

#define WIFI_LOOKUP_BASE_URI "https://maps.googleapis.com/maps/api/browserlocation/json?browser=firefox&sensor=true"

static const char *attribution_text = "This product includes GeoLite data created by MaxMind, available from http://www.maxmind.com";

static char *error_message_array [] = {
        "Invalid IP address '%s'",
        "Can not find the IP address '%s' in the database",
        "Can not open GeoLiteCity/GeoIP Binary database. Set GEOIP_DATABASE_PATH env variable."
};

static void
print_error_in_json (GeoipServerError  error_code,
                     const char       *extra_info)
{
        g_print ("{\"error_code\":%d, \"error_message\":\"", error_code);
        if (extra_info)
                g_print (error_message_array[error_code], extra_info);
        else
                g_print (error_message_array[error_code]);
        g_print ("\"}\n");
}

static JsonBuilder*
add_result_attr_to_json_tree (const char *ipaddress,
                              GeoIP      *gi)
{
        const char *timezone = NULL;
        JsonBuilder *builder;
        GeoIPRecord *gir;

        gir = GeoIP_record_by_addr (gi, ipaddress);
        if (gir == NULL) {
                print_error_in_json (INVALID_ENTRY_ERR, ipaddress);
                return NULL;
        }

        builder = json_builder_new ();

        json_builder_begin_object (builder); /* begin */

        json_builder_set_member_name (builder, "ip");
        json_builder_add_string_value (builder, ipaddress);

        json_builder_set_member_name (builder, "latitude");
        json_builder_add_double_value (builder, gir->latitude);
        json_builder_set_member_name (builder, "longitude");
        json_builder_add_double_value (builder, gir->longitude);

        const char *accuracy = "country";

        /* Country level information */
        if (gir->country_name != NULL) {
                json_builder_set_member_name (builder, "country_name");
                json_builder_add_string_value (builder, gir->country_name);
        }
        if (gir->country_code != NULL) {
                json_builder_set_member_name (builder, "country_code");
                json_builder_add_string_value (builder, gir->country_code);

                /* Region level information */
                if (gir->region != NULL) {
                    accuracy = "region";
                    json_builder_set_member_name (builder, "region_name");
                    json_builder_add_string_value (builder,
                                                   GeoIP_region_name_by_code (gir->country_code, gir->region));
                }
        }
        if (gir->area_code > 0) {
                json_builder_set_member_name (builder, "areacode");
                json_builder_add_int_value (builder, gir->area_code);
        }

        /* City level information */
        if (gir->city != NULL) {
                accuracy = "city";
                json_builder_set_member_name (builder, "city");
                json_builder_add_string_value (builder, gir->city);
        }
        if (gir->postal_code != NULL) {
                json_builder_set_member_name (builder, "zipcode");
                json_builder_add_string_value (builder, gir->postal_code);
        }
        if (gir->metro_code > 0) {
                json_builder_set_member_name (builder, "metro_code");
                json_builder_add_int_value (builder, gir->metro_code);
        }

        json_builder_set_member_name (builder, "accuracy");
        json_builder_add_string_value (builder, accuracy);

        timezone = GeoIP_time_zone_by_country_and_region(gir->country_code, gir->region);
        if (timezone) {
                json_builder_set_member_name (builder, "timezone");
                json_builder_add_string_value (builder, timezone);
        }

        json_builder_set_member_name (builder, "attribution");
        json_builder_add_string_value (builder, attribution_text);

        json_builder_end_object (builder); /* end */

        GeoIPRecord_delete (gir);

        return builder;
}

static JsonBuilder*
add_result_attr_to_json_tree_geoipdb (const char *ipaddress,
                                      GeoIP      *gi)
{
        JsonBuilder *builder;
        const char *country_name;
        const char *country_code;

        country_code = GeoIP_country_code_by_addr (gi, ipaddress);
        country_name = GeoIP_country_name_by_addr (gi, ipaddress);

        if (!country_name && !country_code) {
                print_error_in_json (INVALID_ENTRY_ERR, ipaddress);
                return NULL;
        }

        builder = json_builder_new ();

        json_builder_begin_object (builder); /* begin */

        json_builder_set_member_name (builder, "ip");
        json_builder_add_string_value (builder, ipaddress);

        json_builder_set_member_name (builder, "country_code");
        json_builder_add_string_value (builder, country_code);
        json_builder_set_member_name (builder, "country_name");
        json_builder_add_string_value (builder, country_name);

        json_builder_set_member_name (builder, "accuracy");
        json_builder_add_string_value (builder, "country");

        json_builder_set_member_name (builder, "attribution");
        json_builder_add_string_value (builder, attribution_text);

        json_builder_end_object (builder); /* end */

        return builder;
}

static void
print_json_data (JsonBuilder *builder)
{
        JsonNode *node;
        JsonGenerator *generator;
        char *json_data;
        gsize length;

        node = json_builder_get_root (builder);

        generator = json_generator_new ();
        json_generator_set_root (generator, node);
        json_data = json_generator_to_data (generator, &length);
        g_print ("%*s\n", (int) length, json_data);

        g_free (json_data);
        json_node_free (node);
        g_object_unref (generator);
}


static void
ip_addr_lookup (const char *ipaddress)
{
        GeoIP *gi;
        JsonBuilder *builder;
        const char *db_path;
        char *db;
        gboolean using_geoip_db = FALSE;

        db_path = g_getenv ("GEOIP_DATABASE_PATH");
        if (!db_path)
                db_path = GEOIP_DATABASE_PATH ;
        db = g_strconcat (db_path, "/GeoLiteCity.dat", NULL);
        if (g_file_test (db, G_FILE_TEST_EXISTS) == FALSE) {
                g_free (db);
                db = g_strconcat (db_path, "/GeoIP.dat", NULL);
                using_geoip_db = TRUE;
        }

        gi = GeoIP_open (db,  GEOIP_STANDARD | GEOIP_CHECK_CACHE);
        g_free (db);
        if (gi == NULL) {
                print_error_in_json (DATABASE_ERR, NULL);
                return;
        }

        if (using_geoip_db == TRUE)
                builder = add_result_attr_to_json_tree_geoipdb (ipaddress, gi);
        else
                builder = add_result_attr_to_json_tree (ipaddress, gi);

        if (!builder)
                return;

        print_json_data (builder);
        g_object_unref (builder);
        GeoIP_delete (gi);
}

static gboolean
validate_ip_address (const char *ipaddress)
{
        /* TODO : put a check for private ips */
        GInetAddress *inet_address;

        if (!ipaddress)
                return FALSE;

        inet_address = g_inet_address_new_from_string (ipaddress);
        if (inet_address) {
                g_object_unref (inet_address);
                return TRUE;
        }
        return FALSE;
}

static char *
get_client_ipaddress (void)
{
        const char *data;
        char *value;
        guint i;
        static const char *variables[] = {
                "HTTP_CLIENT_IP",
                "HTTP_X_FORWARDED_FOR",
                "HTTP_X_FORWARDED",
                "HTTP_X_CLUSTER_CLIENT_IP",
                "HTTP_FORWARDED_FOR",
                "HTTP_FORWARDED",
                "REMOTE_ADDR",
        };

        for (i = 0; i < G_N_ELEMENTS (variables); i++) {
                data = g_getenv (variables[i]);
                if (g_strcmp0 (variables[i], "HTTP_X_FORWARDED_FOR") == 0) {
                        if (data) {
                                int j;
                                char **data_array;
                                data_array = g_strsplit (data, ",", 0);
                                for (j = 0; data_array[j] != NULL; j++) {
                                        data = data_array[j];
                                        if (validate_ip_address (data) == TRUE) {
                                                value = g_strdup (data);
                                                g_strfreev (data_array);
                                                return value;
                                        }
                                }
                                g_strfreev (data_array);
                        }
                } else {
                        if (validate_ip_address (data) == TRUE)
                                return g_strdup (data);
                }
        }
        print_error_in_json (INVALID_IP_ADDRESS_ERR, NULL);
        return NULL;
}

/* parse_json_for_wifi function parses the JSON output from
 * google web service and converts it into the JSON format
 * used by geocode-glib library. On failure, it returns FALSE.
 * It doesn't print any error message since the IP based search
 * is performed if Wi-Fi based search fails.
 */
static gboolean
parse_json_for_wifi (const char *data)
{
        JsonParser *parser;
        JsonNode *root;
        JsonObject *root_object;
        JsonObject *loc_obj;
        JsonBuilder *builder;
        GError *error;

        parser = json_parser_new ();
        if (json_parser_load_from_data (parser, data, -1, &error) == FALSE) {
                g_object_unref (parser);
                return FALSE;
        }

        root = json_parser_get_root (parser);
        root_object = json_node_get_object (root);

        loc_obj = json_object_get_object_member (root_object, "location");

        if (loc_obj == NULL) {
                return FALSE;
        }
        builder = json_builder_new ();
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "latitude");
        json_builder_add_double_value (builder,
                                       json_object_get_double_member (loc_obj, "lat"));
        json_builder_set_member_name (builder, "longitude");
        json_builder_add_double_value (builder,
                                       json_object_get_double_member (loc_obj, "lng"));


        json_builder_set_member_name (builder, "accuracy");
        json_builder_add_double_value (builder,
                                       json_object_get_double_member (root_object, "accuracy"));

        /* TODO: What should the text of the attribution text be? */
        json_builder_end_object (builder);
        print_json_data (builder);
        g_object_unref (parser);
        g_object_unref (builder);
        return TRUE;
}

static gboolean
wifi_ap_lookup (const char *data)
{
        SoupSession *session;
        SoupMessage *msg;
        char *final_uri;

        session =  soup_session_sync_new ();
        final_uri = g_strconcat (WIFI_LOOKUP_BASE_URI, "&", data, NULL);
        msg = soup_message_new ("GET", final_uri);
        g_free (final_uri);

        soup_session_send_message (session, msg);
        if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
                return parse_json_for_wifi (msg->response_body->data);
        }

        return FALSE;
}

/* Taken from libsoup/soup-form.c */
#define XDIGIT(c) ((c) <= '9' ? (c) - '0' : ((c) & 0x4F) - 'A' + 10)
#define HEXCHAR(s) ((XDIGIT (s[1]) << 4) + XDIGIT (s[2]))

static gboolean
form_decode (char *part)
{
        unsigned char *s, *d;

        s = d = (unsigned char *)part;
        do {
                if (*s == '%') {
                        if (!g_ascii_isxdigit (s[1]) ||
                            !g_ascii_isxdigit (s[2]))
                                return FALSE;
                        *d++ = HEXCHAR (s);
                        s += 2;
                } else if (*s == '+')
                        *d++ = ' ';
                else
                        *d++ = *s;
        } while (*s++);

        return TRUE;
}

/**
 * decode_query_string () returns a hashtable of Wi-Fi AP
 * details and fills the ipaddress variable if there's any
 * ip address in the query string.
 **/
static GHashTable *
decode_query_string (const char *encoded_form,
                     char       **ipaddress)
{
        GHashTable *form_data_set;
        char **pairs, *eq, *name, *value;
        int i;

        form_data_set = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                               g_free, NULL);
        pairs = g_strsplit (encoded_form, "&", -1);
        for (i = 0; pairs[i]; i++) {
                name = pairs[i];

                eq = strchr (name, '=');
                if (!eq)
                        goto end;
                *eq = '\0';
                value = eq + 1;

                if (!form_decode (name) || !form_decode (value))
                        goto end;
                if (strcmp (name, "wifi") == 0) {
                        g_hash_table_insert (form_data_set, name, value);
                        continue;
                } else if (strcmp (name, "ip") == 0) {
                        *ipaddress = g_strdup (value);
                }
end:
                g_free (name);
        }

        g_free (pairs);

        return form_data_set;
}

int
main (void)
{
        GHashTable *ht;
        char *ipaddress = NULL;
        const char *data;

#if (!GLIB_CHECK_VERSION (2, 36, 0))
        g_type_init ();
#endif

        g_print ("Content-type: text/plain;charset=us-ascii\n\n");
        /* If the query string contains a Wi-Fi field, request
         * the information from the Google web service. otherwise
         * use MaxMind's database for an IP-based search.
         * IP-based search is also used as a fallback option
         * when the Wi-Fi based lookup returns an error. */
        data = g_getenv ("QUERY_STRING");
        if (data != NULL) {
                ht = decode_query_string (data, &ipaddress);
                if (g_hash_table_size (ht) > 0) {
                        char *wifi_ap = soup_form_encode_hash (ht);
                        if (wifi_ap_lookup (wifi_ap) != FALSE) {
                                g_hash_table_destroy (ht);
                                g_free (wifi_ap);
                                return 0;
                        }
                        g_free (wifi_ap);
                }
                g_hash_table_destroy (ht);
        }

        if (ipaddress) {
                if (validate_ip_address (ipaddress) == FALSE) {
                        print_error_in_json (INVALID_IP_ADDRESS_ERR, NULL);
                        g_free (ipaddress);
                        return 1;
                }
        } else {
                ipaddress = get_client_ipaddress ();
        }

        if (!ipaddress)
                return 1;

        ip_addr_lookup (ipaddress);
        g_free (ipaddress);

        return 0;
}
