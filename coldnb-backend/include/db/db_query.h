#ifndef COLDNB_DB_QUERY_H
#define COLDNB_DB_QUERY_H

#include "db/db_connection.h"
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>

/* Query result row helper */
typedef struct {
    PGresult *result;
    int row;
} DbRow;

/* Get string value from result row */
const char *db_row_get_string(DbRow *row, const char *column);

/* Get integer value from result row */
int db_row_get_int(DbRow *row, const char *column);

/* Get int64 value from result row */
int64_t db_row_get_int64(DbRow *row, const char *column);

/* Get double value from result row */
double db_row_get_double(DbRow *row, const char *column);

/* Get boolean value from result row */
bool db_row_get_bool(DbRow *row, const char *column);

/* Check if column is NULL */
bool db_row_is_null(DbRow *row, const char *column);

/* Convert result row to JSON object
 * Returns newly created cJSON object (caller must free with cJSON_Delete) */
cJSON *db_row_to_json(DbRow *row);

/* Convert entire result set to JSON array
 * Returns newly created cJSON array (caller must free with cJSON_Delete) */
cJSON *db_result_to_json(PGresult *result);

/* Get number of rows in result */
int db_result_count(PGresult *result);

/* Get number of affected rows for INSERT/UPDATE/DELETE */
int db_result_affected(PGresult *result);

/* Get single value from result (row 0, column 0) */
const char *db_result_value(PGresult *result);

/* Check if result has rows */
bool db_result_has_rows(PGresult *result);

/* Query builder helpers */

/* Build INSERT query
 * columns and values are parallel arrays
 * Returns newly allocated query string (caller must free) */
char *db_build_insert(const char *table, const char **columns,
                      int count, bool returning_id);

/* Build UPDATE query
 * Returns newly allocated query string (caller must free) */
char *db_build_update(const char *table, const char **columns,
                      int count, const char *where);

/* Build SELECT query with basic filtering
 * Returns newly allocated query string (caller must free) */
char *db_build_select(const char *table, const char **columns, int count,
                      const char *where, const char *order_by,
                      int limit, int offset);

/* Build pagination info from count query and parameters */
typedef struct {
    int page;
    int per_page;
    int total;
    int total_pages;
    int offset;
} DbPagination;

void db_build_pagination(DbPagination *pag, int page, int per_page, int total);

/* Execute an INSERT and return the generated ID */
int64_t db_insert_returning_id(PGconn *conn, const char *query,
                               int nParams, const char *const *paramValues);

/* Execute a COUNT query and return the count */
int db_count(PGconn *conn, const char *query,
             int nParams, const char *const *paramValues);

/* Check if a record exists */
bool db_exists(PGconn *conn, const char *query,
               int nParams, const char *const *paramValues);

#endif /* COLDNB_DB_QUERY_H */
