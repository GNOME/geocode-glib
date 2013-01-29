/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <GeoIP.h>
#include <GeoIPCity.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

static const char *attribution_text = "This product includes GeoLite data created by MaxMind, available from http://www.maxmind.com\n";

enum ERROR_CODES {
        INVALID_IP_ADDRESS_ERR = 0,
        INVALID_ENTRY_ERR,
        DATABASE_ERR
};
static char *error_message_array [] = {
        "Invalid IP address input",
        "Can not find the IP address in the database",
        "Can not open the GeoLiteCity Binary database. Set GEOIP_DATABASE_PATH env variable."
};

static void
add_json_object_for_address (JsonBuilder *builder,
                             const char  *name,
                             const char  *code,
                             const char  *type)
{
        if (!name && !code)
                return;
        json_builder_begin_object (builder);
        if (name) {
                json_builder_set_member_name (builder, "name");
                json_builder_add_string_value (builder, name);
        }
        if (code) {
                json_builder_set_member_name (builder, "code");
                json_builder_add_string_value (builder, code);
        }
        json_builder_set_member_name (builder, "type");
        json_builder_add_string_value (builder, type);
        json_builder_end_object (builder);
}

static void
print_error_in_json (int        error_code,
                     const char *extra_info)
{
        g_print ("{\"results\":\"%s",error_message_array[error_code]);
        if (extra_info)
                g_print (" - %s\"", extra_info);
        else
                g_print ("\"");
        g_print (",\"status\":\"ERROR\"}\n");
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

        json_builder_set_member_name (builder, "results");
        json_builder_begin_object (builder); /* begin results object */

        json_builder_set_member_name (builder, ipaddress);
        json_builder_begin_array (builder); /* begin ipaddress array */

        json_builder_begin_object (builder); /* begin ipaddress object */

        json_builder_set_member_name (builder, "location");
        json_builder_begin_object (builder); /* begin location object */
        json_builder_set_member_name (builder, "latitude");
        json_builder_add_double_value (builder, gir->latitude);
        json_builder_set_member_name (builder, "longitude");
        json_builder_add_double_value (builder, gir->longitude);
        json_builder_end_object (builder); /* end location object */

        json_builder_set_member_name (builder, "address");
        json_builder_begin_array (builder); /* begin address array */

        /* Before adding any entry check if that's NULL.
           If NULL then don't add it to the JSON output.
        */
        add_json_object_for_address (builder, gir->city, NULL, "city");
        add_json_object_for_address (builder,
                                     GeoIP_region_name_by_code (gir->country_code, gir->region),
                                     gir->region,
                                     "region");
        add_json_object_for_address (builder, NULL, gir->postal_code, "postalcode");
        add_json_object_for_address (builder, gir->country_name, gir->country_code, "country");

        json_builder_end_array (builder); /* end address array  */

        timezone = GeoIP_time_zone_by_country_and_region(gir->country_code, gir->region);
        if (timezone) {
                json_builder_set_member_name (builder, "timezone");
                json_builder_add_string_value (builder, timezone);
        }

        json_builder_set_member_name (builder, "accuracy");
        json_builder_add_string_value (builder, "city");

        json_builder_set_member_name (builder, "attribution");
        json_builder_add_string_value (builder, attribution_text);

        json_builder_end_object (builder); /* end ipaddress object */

        json_builder_end_array (builder); /* end ipaddress array */

        json_builder_end_object (builder); /* end results object */

        json_builder_set_member_name (builder, "status");
        json_builder_add_string_value (builder, "OK");

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

        json_builder_set_member_name (builder, "results");
        json_builder_begin_object (builder); /* begin results object */

        json_builder_set_member_name (builder, ipaddress);
        json_builder_begin_array (builder); /* begin ipaddress array */
        json_builder_begin_object (builder); /* begin ipaddress object */

        json_builder_set_member_name (builder, "address");
        json_builder_begin_array (builder); /* begin address array */

        /* Before adding any entry check if that's NULL.
           If NULL then don't add it to the JSON output.
        */
        add_json_object_for_address (builder,
                                     GeoIP_country_name_by_addr (gi, ipaddress),
                                     GeoIP_country_code_by_addr (gi, ipaddress),
                                     "country");

        json_builder_end_array (builder); /* end address array  */

        json_builder_set_member_name (builder, "accuracy");
        json_builder_add_string_value (builder, "country");

        json_builder_set_member_name (builder, "attribution");
        json_builder_add_string_value (builder, attribution_text);

        json_builder_end_object (builder); /* end ipaddress object */

        json_builder_end_array (builder); /* end ipaddress array */

        json_builder_end_object (builder); /* end results object */

        json_builder_set_member_name (builder, "status");
        json_builder_add_string_value (builder, "OK");

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
        const char *db;
        gboolean using_geoip_db = FALSE;

        /* TODO : the server expects a GeoLiteCity database in
           the GEOIP_DATABASE_PATH env variable. If the env var
           gives a GeoIp database it will return an error even
           though the server is capable of handling the latter.
         */
        db = g_getenv ("GEOIP_DATABASE_PATH");
        if (!db)
                db = GEOIP_DATABASE_PATH "/GeoLiteCity.dat";

        if (g_file_test (db, G_FILE_TEST_EXISTS) == FALSE) {
                db = GEOIP_DATABASE_PATH "/GeoIP.dat";
                using_geoip_db = TRUE;
        }

        gi = GeoIP_open (db,  GEOIP_STANDARD | GEOIP_CHECK_CACHE);
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
get_ipaddress_from_query (void)
{
        GHashTable *table;
        const char *data;
        char *value;

        data = g_getenv ("QUERY_STRING");
        if (data == NULL)
                return NULL;

        table = soup_form_decode (data);
        value = g_strdup (g_hash_table_lookup (table, "ip"));
        g_hash_table_destroy (table);

        if (validate_ip_address (value))
                return value;

        g_free (value);

        return NULL;
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
        return NULL;
}

static char *
get_ipaddress (void)
{
        char *value;

        value = get_ipaddress_from_query ();
        if (value) {
                if (validate_ip_address (value) == FALSE) {
                        print_error_in_json (INVALID_IP_ADDRESS_ERR, NULL);
                        g_free (value);
                        return NULL;
                }
        }
        else {
                value = get_client_ipaddress ();
                if (!value) {
                        print_error_in_json (INVALID_IP_ADDRESS_ERR, NULL);
                        g_free (value);
                        return NULL;
                }
        }

        return value;
}

int
main (void)
{
        char *ipaddress;

        g_type_init ();

        g_print ("Content-type: text/plain;charset=us-ascii\n\n");
        ipaddress = get_ipaddress ();
        if (!ipaddress)
                return 0;

        ip_addr_lookup (ipaddress);

        g_free (ipaddress);
        return 0;
}
