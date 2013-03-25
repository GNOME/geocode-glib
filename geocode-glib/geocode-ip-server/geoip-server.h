/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef GEOIP_SERVER_H
#define GEOIP_SERVER_H

#include <glib.h>

typedef enum {
        INVALID_IP_ADDRESS_ERR = 0,
        INVALID_ENTRY_ERR,
        DATABASE_ERR
} GeoipServerError;

#endif /* GEOIP_SERVER_H */
