/*-------------------------------------------------------------------------
 *
 * clickhousedb_internal.h
 *		internal function for clickhousedb_fdw
 *
 * Portions Copyright © 2018-2020, Percona LLC and/or its affiliates
 *
 * Portions Copyright © 2019-2020, Adjust GmbH
 *
 * Portions Copyright © 1996-2020, PostgreSQL Global Development Group
 *
 * Portions Copyright © 1994, The Regents of the University of California
 *
 * IDENTIFICATION
 *		contrib/clickhousedb_fdw/include/clickhousedb_internal.h
 *
 *-------------------------------------------------------------------------
 */


#ifndef CLICKHOUSE_INTERNAL_H
#define CLICKHOUSE_INTERNAL_H

#include "curl/curl.h"

typedef struct ch_http_connection_t
{
	CURL			   *curl;
	char			   *base_url;
	size_t				base_url_len;
} ch_http_connection_t;

typedef struct ch_binary_connection_t
{
	void			  *client;
	void			  *options;
	char			  *error;
} ch_binary_connection_t;

#endif
