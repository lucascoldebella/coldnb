#include "db/db_connection.h"
#include "log/log.h"
#include "util/string_util.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* Connection entry in pool */
typedef struct {
    PGconn *conn;
    bool in_use;
} PoolEntry;

/* Pool structure */
struct DbPool {
    DbPoolConfig config;
    PoolEntry *entries;
    int size;
    pthread_mutex_t lock;
    pthread_cond_t available;
    bool shutdown;
};

/* Create a single connection */
static PGconn *create_connection(const DbPoolConfig *config) {
    char conninfo[1024];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%d dbname=%s user=%s password=%s connect_timeout=%d",
             config->host ? config->host : "localhost",
             config->port > 0 ? config->port : 5432,
             config->dbname ? config->dbname : "coldnb",
             config->user ? config->user : "coldnb",
             config->password ? config->password : "",
             config->connect_timeout > 0 ? config->connect_timeout : 10);

    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        LOG_ERROR("Database connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }

    /* Set client encoding to UTF-8 */
    PQsetClientEncoding(conn, "UTF8");

    return conn;
}

/* Check if connection is still alive */
static bool connection_alive(PGconn *conn) {
    if (conn == NULL) {
        return false;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        return false;
    }

    /* Send a simple query to verify */
    PGresult *result = PQexec(conn, "SELECT 1");
    bool ok = PQresultStatus(result) == PGRES_TUPLES_OK;
    PQclear(result);

    return ok;
}

DbPool *db_pool_create(const DbPoolConfig *config) {
    if (config == NULL) {
        return NULL;
    }

    DbPool *pool = calloc(1, sizeof(DbPool));
    if (pool == NULL) {
        return NULL;
    }

    pool->config = *config;
    pool->size = config->pool_size > 0 ? config->pool_size : 10;

    pool->entries = calloc((size_t)pool->size, sizeof(PoolEntry));
    if (pool->entries == NULL) {
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->entries);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->available, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool->entries);
        free(pool);
        return NULL;
    }

    /* Create initial connections */
    int created = 0;
    for (int i = 0; i < pool->size; i++) {
        pool->entries[i].conn = create_connection(config);
        pool->entries[i].in_use = false;
        if (pool->entries[i].conn != NULL) {
            created++;
        }
    }

    if (created == 0) {
        LOG_ERROR("Failed to create any database connections");
        db_pool_free(pool);
        return NULL;
    }

    LOG_INFO("Database pool created with %d/%d connections", created, pool->size);
    return pool;
}

void db_pool_free(DbPool *pool) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->available);
    pthread_mutex_unlock(&pool->lock);

    /* Close all connections */
    for (int i = 0; i < pool->size; i++) {
        if (pool->entries[i].conn != NULL) {
            PQfinish(pool->entries[i].conn);
        }
    }

    pthread_cond_destroy(&pool->available);
    pthread_mutex_destroy(&pool->lock);
    free(pool->entries);
    free(pool);
}

PGconn *db_pool_acquire(DbPool *pool) {
    if (pool == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&pool->lock);

    while (!pool->shutdown) {
        /* Find an available connection */
        for (int i = 0; i < pool->size; i++) {
            if (!pool->entries[i].in_use && pool->entries[i].conn != NULL) {
                /* Check if connection is still alive */
                if (!connection_alive(pool->entries[i].conn)) {
                    /* Reconnect */
                    PQfinish(pool->entries[i].conn);
                    pool->entries[i].conn = create_connection(&pool->config);
                    if (pool->entries[i].conn == NULL) {
                        continue;
                    }
                }

                pool->entries[i].in_use = true;
                PGconn *conn = pool->entries[i].conn;
                pthread_mutex_unlock(&pool->lock);
                return conn;
            }
        }

        /* No connection available, wait */
        pthread_cond_wait(&pool->available, &pool->lock);
    }

    pthread_mutex_unlock(&pool->lock);
    return NULL;
}

void db_pool_release(DbPool *pool, PGconn *conn) {
    if (pool == NULL || conn == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->lock);

    for (int i = 0; i < pool->size; i++) {
        if (pool->entries[i].conn == conn) {
            /* Reset connection state if in transaction */
            if (PQtransactionStatus(conn) != PQTRANS_IDLE) {
                PGresult *result = PQexec(conn, "ROLLBACK");
                PQclear(result);
            }

            pool->entries[i].in_use = false;
            pthread_cond_signal(&pool->available);
            break;
        }
    }

    pthread_mutex_unlock(&pool->lock);
}

void db_pool_stats(DbPool *pool, DbPoolStats *stats) {
    if (pool == NULL || stats == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->lock);

    stats->total = pool->size;
    stats->available = 0;
    stats->in_use = 0;

    for (int i = 0; i < pool->size; i++) {
        if (pool->entries[i].conn != NULL) {
            if (pool->entries[i].in_use) {
                stats->in_use++;
            } else {
                stats->available++;
            }
        }
    }

    pthread_mutex_unlock(&pool->lock);
}

bool db_pool_healthy(DbPool *pool) {
    if (pool == NULL) {
        return false;
    }

    DbPoolStats stats;
    db_pool_stats(pool, &stats);

    return (stats.available + stats.in_use) > 0;
}

PGresult *db_exec(PGconn *conn, const char *query) {
    if (conn == NULL || query == NULL) {
        return NULL;
    }

    PGresult *result = PQexec(conn, query);

    if (!db_result_ok(result)) {
        LOG_ERROR("Query failed: %s", PQerrorMessage(conn));
        LOG_DEBUG("Failed query: %s", query);
    }

    return result;
}

PGresult *db_exec_params(PGconn *conn, const char *query,
                         int nParams, const char *const *paramValues) {
    if (conn == NULL || query == NULL) {
        return NULL;
    }

    PGresult *result = PQexecParams(conn, query, nParams,
                                    NULL,          /* paramTypes */
                                    paramValues,
                                    NULL,          /* paramLengths */
                                    NULL,          /* paramFormats */
                                    0);            /* resultFormat (text) */

    if (!db_result_ok(result)) {
        LOG_ERROR("Parameterized query failed: %s", PQerrorMessage(conn));
        LOG_DEBUG("Failed query: %s", query);
    }

    return result;
}

bool db_begin(PGconn *conn) {
    PGresult *result = db_exec(conn, "BEGIN");
    bool ok = db_result_ok(result);
    PQclear(result);
    return ok;
}

bool db_commit(PGconn *conn) {
    PGresult *result = db_exec(conn, "COMMIT");
    bool ok = db_result_ok(result);
    PQclear(result);
    return ok;
}

bool db_rollback(PGconn *conn) {
    PGresult *result = db_exec(conn, "ROLLBACK");
    bool ok = db_result_ok(result);
    PQclear(result);
    return ok;
}

bool db_result_ok(PGresult *result) {
    if (result == NULL) {
        return false;
    }

    ExecStatusType status = PQresultStatus(result);
    return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
}

const char *db_error_message(PGconn *conn) {
    if (conn == NULL) {
        return "No connection";
    }
    return PQerrorMessage(conn);
}

char *db_escape_string(PGconn *conn, const char *str) {
    if (conn == NULL || str == NULL) {
        return NULL;
    }

    size_t len = strlen(str);
    char *escaped = malloc(len * 2 + 1);
    if (escaped == NULL) {
        return NULL;
    }

    int error = 0;
    PQescapeStringConn(conn, escaped, str, len, &error);

    if (error) {
        free(escaped);
        return NULL;
    }

    return escaped;
}

char *db_escape_identifier(PGconn *conn, const char *str) {
    if (conn == NULL || str == NULL) {
        return NULL;
    }

    char *escaped = PQescapeIdentifier(conn, str, strlen(str));
    if (escaped == NULL) {
        return NULL;
    }

    /* PQescapeIdentifier returns PQ-allocated memory, copy it */
    char *result = str_dup(escaped);
    PQfreemem(escaped);

    return result;
}
