#ifndef COLDNB_DB_CONNECTION_H
#define COLDNB_DB_CONNECTION_H

#include <libpq-fe.h>
#include <stdbool.h>
#include <stddef.h>

/* Database pool configuration */
typedef struct {
    const char *host;
    int port;
    const char *dbname;
    const char *user;
    const char *password;
    int pool_size;
    int connect_timeout;      /* Seconds */
} DbPoolConfig;

/* Opaque pool handle */
typedef struct DbPool DbPool;

/* Create a connection pool
 * Returns NULL on error */
DbPool *db_pool_create(const DbPoolConfig *config);

/* Free the connection pool
 * Closes all connections */
void db_pool_free(DbPool *pool);

/* Get a connection from the pool
 * Blocks if no connections available
 * Returns NULL on error
 * Caller must release with db_pool_release */
PGconn *db_pool_acquire(DbPool *pool);

/* Return a connection to the pool */
void db_pool_release(DbPool *pool, PGconn *conn);

/* Get pool statistics */
typedef struct {
    int total;
    int available;
    int in_use;
} DbPoolStats;

void db_pool_stats(DbPool *pool, DbPoolStats *stats);

/* Check if pool is healthy */
bool db_pool_healthy(DbPool *pool);

/* Execute a simple query with no parameters
 * Returns NULL on error (caller must PQclear result) */
PGresult *db_exec(PGconn *conn, const char *query);

/* Execute a parameterized query
 * Returns NULL on error (caller must PQclear result) */
PGresult *db_exec_params(PGconn *conn, const char *query,
                         int nParams, const char *const *paramValues);

/* Begin a transaction */
bool db_begin(PGconn *conn);

/* Commit a transaction */
bool db_commit(PGconn *conn);

/* Rollback a transaction */
bool db_rollback(PGconn *conn);

/* Check if result is OK (PGRES_COMMAND_OK or PGRES_TUPLES_OK) */
bool db_result_ok(PGresult *result);

/* Get error message from connection or result */
const char *db_error_message(PGconn *conn);

/* Escape string for SQL (caller must free returned string) */
char *db_escape_string(PGconn *conn, const char *str);

/* Escape identifier (table/column name) */
char *db_escape_identifier(PGconn *conn, const char *str);

#endif /* COLDNB_DB_CONNECTION_H */
