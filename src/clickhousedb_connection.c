/*-------------------------------------------------------------------------
 *
 * clickhousedb_connection.c
 *		  Connection management functions for clickhousedb_fdw
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
 *		  contrib/clickhousedb_fdw/clickhousedb_connection.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_user_mapping.h"
#include "access/xact.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "storage/latch.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/inval.h"
#include "utils/memutils.h"
#include "utils/syscache.h"

#include "clickhousedb_fdw.h"

/*
 * Connection cache (initialized on first use)
 */
static HTAB *ConnectionHash = NULL;
static List *cursors = NIL;
static void chfdw_inval_callback(Datum arg, int cacheid, uint32 hashvalue);


static ch_connection
clickhouse_connect(ForeignServer *server, UserMapping *user)
{
	char	   *connstring;
	char	   *driver = "http";

	/* default settings */
	ch_connection_details	details = {"127.0.0.1", 8123, NULL, NULL, "default"};

	chfdw_extract_options(server->options, &driver, &details.host,
		&details.port, &details.dbname, &details.username, &details.password);
	chfdw_extract_options(user->options, &driver, &details.host,
		&details.port, &details.dbname, &details.username, &details.password);

	if (strcmp(driver, "http") == 0)
	{
		return chfdw_http_connect(&details);
	}
	else if (strcmp(driver, "binary") == 0)
	{
		if (details.port == 8123)
			details.port = 9000;

		return chfdw_binary_connect(&details);
	}
	else
		elog(ERROR, "invalid ClickHouse connection driver");
}

ch_connection
chfdw_get_connection(UserMapping *user)
{
	bool		found;
	ConnCacheEntry *entry;
	ConnCacheKey key;

	/* First time through, initialize connection cache hashtable */
	if (ConnectionHash == NULL)
	{
		HASHCTL		ctl;

		MemSet(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(ConnCacheKey);
		ctl.entrysize = sizeof(ConnCacheEntry);
		/* allocate ConnectionHash in the cache context */
		ctl.hcxt = CacheMemoryContext;
		ConnectionHash = hash_create("clickhousedb_fdw connections", 8,
									 &ctl,
									 HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);

		/*
		 * Register some callback functions that manage connection cleanup.
		 * This should be done just once in each backend.
		 */
		CacheRegisterSyscacheCallback(FOREIGNSERVEROID,
									  chfdw_inval_callback, (Datum) 0);
		CacheRegisterSyscacheCallback(USERMAPPINGOID,
									  chfdw_inval_callback, (Datum) 0);
	}

	/* Create hash key for the entry.  Assume no pad bytes in key struct */
	key.userid = user->umid;

	/*
	 * Find or create cached entry for requested connection.
	 */
	entry = hash_search(ConnectionHash, &key, HASH_ENTER, &found);
	if (!found)
	{
		/*
		 * We need only clear "conn" here; remaining fields will be filled
		 * later when "conn" is set.
		 */
		entry->gate.conn = NULL;
	}

	/*
	 * If the connection needs to be remade due to invalidation, disconnect as
	 * soon as we're out of all transactions.
	 */
	if (entry->gate.conn != NULL && entry->invalidated)
	{
		elog(LOG, "closing connection to ClickHouse due to invalidation");
		entry->gate.methods->disconnect(entry->gate.conn);
		entry->gate.conn = NULL;
	}

	/*
	 * We don't check the health of cached connection here, because it would
	 * require some overhead.  Broken connection will be detected when the
	 * connection is actually used.
	 */

	/*
	 * If cache entry doesn't have a connection, we have to establish a new
	 * connection.  (If clickhouse_connect throws an error, the cache entry
	 * will remain in a valid empty state, ie conn == NULL.)
	 */
	if (entry->gate.conn == NULL)
	{
		ForeignServer *server = GetForeignServer(user->serverid);

		/* Reset all transient state fields, to be sure all are clean */
		entry->invalidated = false;
		entry->server_hashvalue =
			GetSysCacheHashValue1(FOREIGNSERVEROID,
								  ObjectIdGetDatum(server->serverid));
		entry->mapping_hashvalue =
			GetSysCacheHashValue1(USERMAPPINGOID,
								  ObjectIdGetDatum(user->umid));

		/* Now try to make the connection */
		entry->gate = clickhouse_connect(server, user);

		elog(DEBUG3,
			 "new clickhousedb_fdw connection %p for server \"%s\" (user mapping oid %u, userid %u)",
			 entry->gate.conn, server->servername, user->umid, user->userid);
	}

	return entry->gate;
}

/*
 * Connection invalidation callback function
 *
 * After a change to a pg_foreign_server or pg_user_mapping catalog entry,
 * mark connections depending on that entry as needing to be remade.
 * We can't immediately destroy them, since they might be in the midst of
 * a transaction, but we'll remake them at the next opportunity.
 *
 * Although most cache invalidation callbacks blow away all the related stuff
 * regardless of the given hashvalue, connections are expensive enough that
 * it's worth trying to avoid that.
 *
 * NB: We could avoid unnecessary disconnection more strictly by examining
 * individual option values, but it seems too much effort for the gain.
 */
static void
chfdw_inval_callback(Datum arg, int cacheid, uint32 hashvalue)
{
	HASH_SEQ_STATUS scan;
	ConnCacheEntry *entry;

	Assert(cacheid == FOREIGNSERVEROID || cacheid == USERMAPPINGOID);

	/* ConnectionHash must exist already, if we're registered */
	hash_seq_init(&scan, ConnectionHash);
	while ((entry = (ConnCacheEntry *) hash_seq_search(&scan)))
	{
		/* Ignore empty entries */
		if (entry->gate.conn == NULL)
			continue;

		/* hashvalue == 0 means a cache reset, must clear all state */
		if (hashvalue == 0 ||
				(cacheid == FOREIGNSERVEROID &&
				 entry->server_hashvalue == hashvalue) ||
				(cacheid == USERMAPPINGOID &&
				 entry->mapping_hashvalue == hashvalue))
		{
			entry->invalidated = true;
		}
	}
}


ch_connection_details *
connstring_parse(const char *connstring)
{
	char	   *pname;
	char	   *pval;
	char	   *buf;
	char	   *cp;
	char	   *cp2;
    ch_connection_details *details = palloc0(sizeof(ch_connection_details));

	/* Need a modifiable copy of the input string */
	if ((buf = pstrdup(connstring)) == NULL)
		return NULL;

	cp = buf;

	while (*cp)
	{
		/* Skip blanks before the parameter name */
		if (isspace((unsigned char) *cp))
		{
			cp++;
			continue;
		}

		/* Get the parameter name */
		pname = cp;
		while (*cp)
		{
			if (*cp == '=')
				break;
			if (isspace((unsigned char) *cp))
			{
				*cp++ = '\0';
				while (*cp)
				{
					if (!isspace((unsigned char) *cp))
						break;
					cp++;
				}
				break;
			}
			cp++;
		}

		/* Check that there is a following '=' */
		if (*cp != '=')
			elog(ERROR, "missing \"=\" after \"%s\" in connection info string", pname);

		*cp++ = '\0';

		/* Skip blanks after the '=' */
		while (*cp)
		{
			if (!isspace((unsigned char) *cp))
				break;
			cp++;
		}

		/* Get the parameter value */
		pval = cp;

		if (*cp != '\'')
		{
			cp2 = pval;
			while (*cp)
			{
				if (isspace((unsigned char) *cp))
				{
					*cp++ = '\0';
					break;
				}
				if (*cp == '\\')
				{
					cp++;
					if (*cp != '\0')
						*cp2++ = *cp++;
				}
				else
					*cp2++ = *cp++;
			}
			*cp2 = '\0';
		}
		else
		{
			cp2 = pval;
			cp++;
			for (;;)
			{
				if (*cp == '\0')
					elog(ERROR, "unterminated quoted string in connection info string");

				if (*cp == '\\')
				{
					cp++;
					if (*cp != '\0')
						*cp2++ = *cp++;
					continue;
				}
				if (*cp == '\'')
				{
					*cp2 = '\0';
					cp++;
					break;
				}
				*cp2++ = *cp++;
			}
		}

		if (strcmp(pname, "host") == 0) {
			details->host = pstrdup(pval);
		} else if (strcmp(pname, "port") == 0) {
			details->port = pg_strtoint32(pval);
		} else if (strcmp(pname, "username") == 0) {
			details->username = pstrdup(pval);
		} else if (strcmp(pname, "password") == 0) {
			details->password = pstrdup(pval);
		}
	}

    pfree(buf);
    return details;
}
