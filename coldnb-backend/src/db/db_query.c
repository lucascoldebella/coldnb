#include "db/db_query.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Get column index by name */
static int get_column_index(PGresult *result, const char *column) {
    if (result == NULL || column == NULL) {
        return -1;
    }
    return PQfnumber(result, column);
}

const char *db_row_get_string(DbRow *row, const char *column) {
    if (row == NULL || row->result == NULL) {
        return NULL;
    }

    int col = get_column_index(row->result, column);
    if (col < 0) {
        return NULL;
    }

    if (PQgetisnull(row->result, row->row, col)) {
        return NULL;
    }

    return PQgetvalue(row->result, row->row, col);
}

int db_row_get_int(DbRow *row, const char *column) {
    const char *value = db_row_get_string(row, column);
    if (value == NULL) {
        return 0;
    }
    return atoi(value);
}

int64_t db_row_get_int64(DbRow *row, const char *column) {
    const char *value = db_row_get_string(row, column);
    if (value == NULL) {
        return 0;
    }
    return strtoll(value, NULL, 10);
}

double db_row_get_double(DbRow *row, const char *column) {
    const char *value = db_row_get_string(row, column);
    if (value == NULL) {
        return 0.0;
    }
    return strtod(value, NULL);
}

bool db_row_get_bool(DbRow *row, const char *column) {
    const char *value = db_row_get_string(row, column);
    if (value == NULL) {
        return false;
    }
    return value[0] == 't' || value[0] == 'T' || value[0] == '1';
}

bool db_row_is_null(DbRow *row, const char *column) {
    if (row == NULL || row->result == NULL) {
        return true;
    }

    int col = get_column_index(row->result, column);
    if (col < 0) {
        return true;
    }

    return PQgetisnull(row->result, row->row, col) == 1;
}

cJSON *db_row_to_json(DbRow *row) {
    if (row == NULL || row->result == NULL) {
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    int ncols = PQnfields(row->result);
    for (int i = 0; i < ncols; i++) {
        const char *name = PQfname(row->result, i);

        if (PQgetisnull(row->result, row->row, i)) {
            cJSON_AddNullToObject(obj, name);
            continue;
        }

        const char *value = PQgetvalue(row->result, row->row, i);
        Oid type = PQftype(row->result, i);

        /* Determine type based on PostgreSQL OID */
        switch (type) {
            case 16:   /* bool */
                cJSON_AddBoolToObject(obj, name, value[0] == 't');
                break;

            case 20:   /* int8 */
            case 21:   /* int2 */
            case 23:   /* int4 */
                cJSON_AddNumberToObject(obj, name, strtoll(value, NULL, 10));
                break;

            case 700:  /* float4 */
            case 701:  /* float8 */
            case 1700: /* numeric */
                cJSON_AddNumberToObject(obj, name, strtod(value, NULL));
                break;

            case 114:  /* json */
            case 3802: /* jsonb */
                {
                    cJSON *json = cJSON_Parse(value);
                    if (json != NULL) {
                        cJSON_AddItemToObject(obj, name, json);
                    } else {
                        cJSON_AddStringToObject(obj, name, value);
                    }
                }
                break;

            default:   /* Everything else as string */
                cJSON_AddStringToObject(obj, name, value);
                break;
        }
    }

    return obj;
}

cJSON *db_result_to_json(PGresult *result) {
    if (result == NULL) {
        return NULL;
    }

    cJSON *arr = cJSON_CreateArray();
    if (arr == NULL) {
        return NULL;
    }

    int nrows = PQntuples(result);
    for (int i = 0; i < nrows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *obj = db_row_to_json(&row);
        if (obj != NULL) {
            cJSON_AddItemToArray(arr, obj);
        }
    }

    return arr;
}

int db_result_count(PGresult *result) {
    if (result == NULL) {
        return 0;
    }
    return PQntuples(result);
}

int db_result_affected(PGresult *result) {
    if (result == NULL) {
        return 0;
    }
    const char *affected = PQcmdTuples(result);
    if (affected == NULL || affected[0] == '\0') {
        return 0;
    }
    return atoi(affected);
}

const char *db_result_value(PGresult *result) {
    if (result == NULL || PQntuples(result) == 0 || PQnfields(result) == 0) {
        return NULL;
    }
    if (PQgetisnull(result, 0, 0)) {
        return NULL;
    }
    return PQgetvalue(result, 0, 0);
}

bool db_result_has_rows(PGresult *result) {
    return db_result_count(result) > 0;
}

char *db_build_insert(const char *table, const char **columns,
                      int count, bool returning_id) {
    if (table == NULL || columns == NULL || count <= 0) {
        return NULL;
    }

    /* Calculate buffer size */
    size_t size = 256;
    for (int i = 0; i < count; i++) {
        size += strlen(columns[i]) + 10;
    }

    char *query = malloc(size);
    if (query == NULL) {
        return NULL;
    }

    /* Build column list and placeholders */
    char col_list[2048];
    char val_list[2048];
    size_t col_len = 0;
    size_t val_len = 0;
    col_list[0] = '\0';
    val_list[0] = '\0';

    for (int i = 0; i < count; i++) {
        if (i > 0) {
            col_len += (size_t)snprintf(col_list + col_len, sizeof(col_list) - col_len, ", ");
            val_len += (size_t)snprintf(val_list + val_len, sizeof(val_list) - val_len, ", ");
        }
        col_len += (size_t)snprintf(col_list + col_len, sizeof(col_list) - col_len, "%s", columns[i]);
        val_len += (size_t)snprintf(val_list + val_len, sizeof(val_list) - val_len, "$%d", i + 1);
    }
    (void)col_len;
    (void)val_len;

    if (returning_id) {
        snprintf(query, size, "INSERT INTO %s (%s) VALUES (%s) RETURNING id",
                 table, col_list, val_list);
    } else {
        snprintf(query, size, "INSERT INTO %s (%s) VALUES (%s)",
                 table, col_list, val_list);
    }

    return query;
}

char *db_build_update(const char *table, const char **columns,
                      int count, const char *where) {
    if (table == NULL || columns == NULL || count <= 0) {
        return NULL;
    }

    size_t size = 256 + (where ? strlen(where) : 0);
    for (int i = 0; i < count; i++) {
        size += strlen(columns[i]) + 10;
    }

    char *query = malloc(size);
    if (query == NULL) {
        return NULL;
    }

    /* Build SET clause */
    char set_clause[2048];
    size_t set_len = 0;
    set_clause[0] = '\0';

    for (int i = 0; i < count; i++) {
        if (i > 0) {
            set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len, ", ");
        }
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                                    "%s = $%d", columns[i], i + 1);
    }
    (void)set_len;

    if (where != NULL) {
        snprintf(query, size, "UPDATE %s SET %s WHERE %s", table, set_clause, where);
    } else {
        snprintf(query, size, "UPDATE %s SET %s", table, set_clause);
    }

    return query;
}

char *db_build_select(const char *table, const char **columns, int count,
                      const char *where, const char *order_by,
                      int limit, int offset) {
    size_t size = 512;
    if (where) size += strlen(where);
    if (order_by) size += strlen(order_by);

    char *query = malloc(size);
    if (query == NULL) {
        return NULL;
    }

    /* Build column list */
    char col_list[2048];
    size_t col_len = 0;
    if (columns == NULL || count <= 0) {
        snprintf(col_list, sizeof(col_list), "*");
    } else {
        col_list[0] = '\0';
        for (int i = 0; i < count; i++) {
            if (i > 0) {
                col_len += (size_t)snprintf(col_list + col_len, sizeof(col_list) - col_len, ", ");
            }
            col_len += (size_t)snprintf(col_list + col_len, sizeof(col_list) - col_len, "%s", columns[i]);
        }
    }
    (void)col_len;

    int pos = snprintf(query, size, "SELECT %s FROM %s", col_list, table);

    if (where != NULL) {
        pos += snprintf(query + pos, size - (size_t)pos, " WHERE %s", where);
    }

    if (order_by != NULL) {
        pos += snprintf(query + pos, size - (size_t)pos, " ORDER BY %s", order_by);
    }

    if (limit > 0) {
        pos += snprintf(query + pos, size - (size_t)pos, " LIMIT %d", limit);
    }

    if (offset > 0) {
        snprintf(query + pos, size - (size_t)pos, " OFFSET %d", offset);
    }

    return query;
}

void db_build_pagination(DbPagination *pag, int page, int per_page, int total) {
    if (pag == NULL) {
        return;
    }

    pag->page = page > 0 ? page : 1;
    pag->per_page = per_page > 0 ? per_page : 20;
    pag->total = total > 0 ? total : 0;
    pag->total_pages = (pag->total + pag->per_page - 1) / pag->per_page;
    pag->offset = (pag->page - 1) * pag->per_page;

    /* Clamp page to valid range */
    if (pag->page > pag->total_pages && pag->total_pages > 0) {
        pag->page = pag->total_pages;
        pag->offset = (pag->page - 1) * pag->per_page;
    }
}

int64_t db_insert_returning_id(PGconn *conn, const char *query,
                               int nParams, const char *const *paramValues) {
    PGresult *result = db_exec_params(conn, query, nParams, paramValues);
    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        return -1;
    }

    const char *id_str = db_result_value(result);
    int64_t id = id_str ? strtoll(id_str, NULL, 10) : -1;
    PQclear(result);

    return id;
}

int db_count(PGconn *conn, const char *query,
             int nParams, const char *const *paramValues) {
    PGresult *result = db_exec_params(conn, query, nParams, paramValues);
    if (!db_result_ok(result)) {
        PQclear(result);
        return 0;
    }

    const char *count_str = db_result_value(result);
    int count = count_str ? atoi(count_str) : 0;
    PQclear(result);

    return count;
}

bool db_exists(PGconn *conn, const char *query,
               int nParams, const char *const *paramValues) {
    return db_count(conn, query, nParams, paramValues) > 0;
}
