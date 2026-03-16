#include "handlers/handler_discounts.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_discounts_register(HttpRouter *router, DbPool *pool) {
    /* Public */
    ROUTE_GET(router, "/api/discount-codes/check", handler_discounts_check, pool);

    /* Admin CRUD */
    ROUTE_GET(router, "/api/admin/discounts", handler_admin_discounts_list, pool);
    ROUTE_POST(router, "/api/admin/discounts", handler_admin_discounts_create, pool);
    ROUTE_PUT(router, "/api/admin/discounts/:id", handler_admin_discounts_update, pool);
    ROUTE_DELETE(router, "/api/admin/discounts/:id", handler_admin_discounts_delete, pool);
}

void handler_discounts_check(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *code = http_request_get_query_param(req, "code");

    if (str_is_empty(code)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "code parameter required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT code, discount_type, discount_value, minimum_order, maximum_discount, "
        "usage_limit, used_count "
        "FROM discount_codes "
        "WHERE code = $1 AND is_active = true "
        "AND (starts_at IS NULL OR starts_at <= NOW()) "
        "AND (expires_at IS NULL OR expires_at > NOW())";

    const char *params[] = { code };
    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        cJSON *data = cJSON_CreateObject();
        cJSON_AddBoolToObject(data, "valid", false);
        cJSON_AddStringToObject(data, "message", "Invalid or expired discount code");
        cJSON *response = json_create_success(data);
        char *json = cJSON_PrintUnformatted(response);
        cJSON_Delete(response);
        http_response_json(resp, HTTP_STATUS_OK, json);
        free(json);
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    const char *discount_type = db_row_get_string(&row, "discount_type");
    double discount_value = db_row_get_double(&row, "discount_value");
    double minimum_order = db_row_get_double(&row, "minimum_order");
    double maximum_discount = db_row_get_double(&row, "maximum_discount");
    int usage_limit = db_row_get_int(&row, "usage_limit");
    int used_count = db_row_get_int(&row, "used_count");

    bool valid = true;
    const char *message = "Discount code is valid";

    if (usage_limit > 0 && used_count >= usage_limit) {
        valid = false;
        message = "This discount code has reached its usage limit";
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "valid", valid);
    cJSON_AddStringToObject(data, "message", message);

    if (valid) {
        cJSON_AddStringToObject(data, "discount_type",
                                discount_type ? discount_type : "percentage");
        cJSON_AddNumberToObject(data, "discount_value", discount_value);
        cJSON_AddNumberToObject(data, "minimum_order", minimum_order);
        if (maximum_discount > 0) {
            cJSON_AddNumberToObject(data, "maximum_discount", maximum_discount);
        }
    }

    PQclear(result);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* GET /api/admin/discounts */
void handler_admin_discounts_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    DbPool *pool = (DbPool *)user_data;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT id, code, description, discount_type, discount_value, "
        "minimum_order, maximum_discount, usage_limit, used_count, "
        "starts_at, expires_at, is_active, created_at "
        "FROM discount_codes ORDER BY created_at DESC";

    PGresult *result = db_exec(conn, query);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *discounts = db_result_to_json(result);
    PQclear(result);

    cJSON *response = json_create_success(discounts);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* POST /api/admin/discounts */
void handler_admin_discounts_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *code = json_get_string(body, "code", NULL);
    const char *description = json_get_string(body, "description", NULL);
    const char *discount_type = json_get_string(body, "discount_type", NULL);

    if (str_is_empty(code) || str_is_empty(discount_type)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "code and discount_type required");
        return;
    }

    /* Copy all strings before freeing body */
    char *c_code = str_dup(code);
    char *c_desc = description ? str_dup(description) : NULL;
    char *c_type = str_dup(discount_type);

    double discount_value = json_get_double(body, "discount_value", 0);
    double minimum_order = json_get_double(body, "minimum_order", 0);
    double maximum_discount = json_get_double(body, "maximum_discount", 0);
    int usage_limit = json_get_int(body, "usage_limit", 0);
    bool is_active = json_get_bool(body, "is_active", true);

    const char *starts_at = json_get_string(body, "starts_at", NULL);
    const char *expires_at = json_get_string(body, "expires_at", NULL);
    char *c_starts = starts_at ? str_dup(starts_at) : NULL;
    char *c_expires = expires_at ? str_dup(expires_at) : NULL;

    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(c_code); free(c_desc); free(c_type);
        free(c_starts); free(c_expires);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char val_s[32], min_s[32], max_s[32], lim_s[32];
    snprintf(val_s, sizeof(val_s), "%.2f", discount_value);
    snprintf(min_s, sizeof(min_s), "%.2f", minimum_order);
    snprintf(max_s, sizeof(max_s), "%.2f", maximum_discount);
    snprintf(lim_s, sizeof(lim_s), "%d", usage_limit);
    const char *active_s = is_active ? "true" : "false";

    const char *query =
        "INSERT INTO discount_codes (code, description, discount_type, discount_value, "
        "minimum_order, maximum_discount, usage_limit, starts_at, expires_at, is_active) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) "
        "RETURNING id, code, description, discount_type, discount_value, "
        "minimum_order, maximum_discount, usage_limit, used_count, "
        "starts_at, expires_at, is_active, created_at";

    const char *params[] = {
        c_code, c_desc, c_type, val_s, min_s, max_s, lim_s,
        c_starts, c_expires, active_s
    };

    PGresult *result = db_exec_params(conn, query, 10, params);
    db_pool_release(pool, conn);

    free(c_code); free(c_desc); free(c_type);
    free(c_starts); free(c_expires);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Failed to create discount code (code may already exist)");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *discount = db_row_to_json(&row);
    PQclear(result);

    cJSON *response = json_create_success(discount);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Discount code created: %s", code);
}

/* PUT /api/admin/discounts/:id */
void handler_admin_discounts_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (str_is_empty(id_str)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Discount ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    /* Build dynamic SET clause — track heap-allocated params for cleanup */
    char set_clause[1024];
    size_t set_len = 0;
    const char *params[12];
    char param_bufs[5][32];  /* For numeric/bool conversions */
    bool param_is_heap[12];
    int param_count = 0;

    memset(param_is_heap, 0, sizeof(param_is_heap));

    const char *code = json_get_string(body, "code", NULL);
    if (code) {
        params[param_count] = str_dup(code);
        param_is_heap[param_count] = true;
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%scode = $%d", set_len > 0 ? ", " : "", param_count);
    }

    const char *description = json_get_string(body, "description", NULL);
    if (description) {
        params[param_count] = str_dup(description);
        param_is_heap[param_count] = true;
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sdescription = $%d", set_len > 0 ? ", " : "", param_count);
    }

    const char *dtype = json_get_string(body, "discount_type", NULL);
    if (dtype) {
        params[param_count] = str_dup(dtype);
        param_is_heap[param_count] = true;
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sdiscount_type = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON *val_item = cJSON_GetObjectItem(body, "discount_value");
    if (val_item && cJSON_IsNumber(val_item)) {
        snprintf(param_bufs[0], sizeof(param_bufs[0]), "%.2f", val_item->valuedouble);
        params[param_count] = param_bufs[0];
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sdiscount_value = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON *min_item = cJSON_GetObjectItem(body, "minimum_order");
    if (min_item && cJSON_IsNumber(min_item)) {
        snprintf(param_bufs[1], sizeof(param_bufs[1]), "%.2f", min_item->valuedouble);
        params[param_count] = param_bufs[1];
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sminimum_order = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON *max_item = cJSON_GetObjectItem(body, "maximum_discount");
    if (max_item && cJSON_IsNumber(max_item)) {
        snprintf(param_bufs[2], sizeof(param_bufs[2]), "%.2f", max_item->valuedouble);
        params[param_count] = param_bufs[2];
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%smaximum_discount = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON *lim_item = cJSON_GetObjectItem(body, "usage_limit");
    if (lim_item && cJSON_IsNumber(lim_item)) {
        snprintf(param_bufs[3], sizeof(param_bufs[3]), "%d", (int)lim_item->valuedouble);
        params[param_count] = param_bufs[3];
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%susage_limit = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON *active_item = cJSON_GetObjectItem(body, "is_active");
    if (active_item && cJSON_IsBool(active_item)) {
        snprintf(param_bufs[4], sizeof(param_bufs[4]), "%s",
                 cJSON_IsTrue(active_item) ? "true" : "false");
        params[param_count] = param_bufs[4];
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sis_active = $%d", set_len > 0 ? ", " : "", param_count);
    }

    const char *starts_at = json_get_string(body, "starts_at", NULL);
    if (starts_at) {
        params[param_count] = str_dup(starts_at);
        param_is_heap[param_count] = true;
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sstarts_at = $%d", set_len > 0 ? ", " : "", param_count);
    }

    const char *expires_at = json_get_string(body, "expires_at", NULL);
    if (expires_at) {
        params[param_count] = str_dup(expires_at);
        param_is_heap[param_count] = true;
        param_count++;
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sexpires_at = $%d", set_len > 0 ? ", " : "", param_count);
    }

    cJSON_Delete(body);

    if (param_count == 0) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "No fields to update");
        return;
    }

    /* Add id as last param */
    params[param_count] = id_str;
    param_count++;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        for (int i = 0; i < param_count; i++) {
            if (param_is_heap[i]) free((char *)params[i]);
        }
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char query[2048];
    snprintf(query, sizeof(query),
             "UPDATE discount_codes SET %s WHERE id = $%d "
             "RETURNING id, code, description, discount_type, discount_value, "
             "minimum_order, maximum_discount, usage_limit, used_count, "
             "starts_at, expires_at, is_active, created_at",
             set_clause, param_count);

    PGresult *result = db_exec_params(conn, query, param_count, params);
    db_pool_release(pool, conn);

    for (int i = 0; i < param_count; i++) {
        if (param_is_heap[i]) free((char *)params[i]);
    }

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Discount code not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *discount = db_row_to_json(&row);
    PQclear(result);

    cJSON *response = json_create_success(discount);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* DELETE /api/admin/discounts/:id */
void handler_admin_discounts_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (str_is_empty(id_str)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Discount ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query = "DELETE FROM discount_codes WHERE id = $1";
    const char *params[] = { id_str };
    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Delete failed");
        return;
    }

    int affected = atoi(PQcmdTuples(result));
    PQclear(result);

    if (affected == 0) {
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Discount code not found");
        return;
    }

    http_response_json(resp, HTTP_STATUS_OK, "{\"success\":true,\"message\":\"Discount code deleted\"}");
}
