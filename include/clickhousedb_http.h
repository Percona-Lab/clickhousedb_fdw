/*-------------------------------------------------------------------------
 *
 * clickhousedb_http.h
 *		HTTP protocol handling for clickhousedb_fdw
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
 *		contrib/clickhousedb_fdw/include/clickhousedb_http.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef CLICKHOUSE_HTTP_H
#define CLICKHOUSE_HTTP_H

#include "postgres.h"
#include "nodes/pg_list.h"
#include "lib/stringinfo.h"

typedef struct ch_http_connection_t ch_http_connection_t;
typedef struct ch_http_response_t
{
	char			   *data;
	size_t				datasize;
	long				http_status;
	char				query_id[37];
	double				pretransfer_time;
	double				total_time;
} ch_http_response_t;

typedef enum
{
	CH_CONT,
	CH_EOL,
	CH_EOF
} ch_read_status;

typedef struct {
	char   *data;
	size_t	buflen;
	size_t	curpos;
	size_t	maxpos;
	char   *val;
	bool	done;
} ch_http_read_state;

typedef struct {
	StringInfoData	sql;
	char		   *sql_begin;		/* beginning part of constructed sql */
	List		   *target_attrs;	/* list of target attribute numbers */
	int				p_nums;			/* number of parameters to transmit */
	ch_http_connection_t *conn;
} ch_http_insert_state;

void ch_http_init(int verbose, uint32_t query_id_prefix);
void ch_http_set_progress_func(void *progressfunc);
ch_http_connection_t *ch_http_connection(char *, int, char *, char *);
void ch_http_close(ch_http_connection_t *conn);
ch_http_response_t *ch_http_simple_query(ch_http_connection_t *conn, const char *query);
char *ch_http_last_error(void);

/* read */
void ch_http_read_state_init(ch_http_read_state *state, char *data, size_t datalen);
void ch_http_read_state_free(ch_http_read_state *state);
int ch_http_read_next(ch_http_read_state *state);
void ch_http_response_free(ch_http_response_t *resp);

#endif
