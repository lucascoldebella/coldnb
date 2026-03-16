#include "handlers/handler_returns.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_returns_register(HttpRouter *router, DbPool *pool) {
    ROUTE_POST(router, "/api/returns", handler_returns_create, pool);
    ROUTE_GET(router, "/api/returns", handler_returns_list, pool);
    ROUTE_GET(router, "/api/returns/:id", handler_returns_get, pool);
}

void handler_admin_returns_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/returns", handler_admin_returns_list, pool);
    ROUTE_PUT(router, "/api/admin/returns/:id/status", handler_admin_returns_update, pool);
}

/*
 * POST /api/returns
 *
 * Body: { "order_id": "uuid", "order_item_id": "uuid" (optional),
 *         "reason": "defective|wrong_item|changed_mind|other",
 *         "description": "..." (optional) }
 */
void handler_returns_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *order_id     = json_get_string(body, "order_id", NULL);
    const char *item_id      = json_get_string(body, "order_item_id", NULL);
    const char *reason       = json_get_string(body, "reason", NULL);
    const char *description  = json_get_string(body, "description", NULL);

    if (str_is_empty(order_id) || str_is_empty(reason)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "order_id and reason are required");
        return;
    }

    if (!uuid_validate(order_id)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid order_id");
        return;
    }

    /* Validate reason value */
    const char *valid_reasons[] = { "defective", "wrong_item", "changed_mind", "other", NULL };
    bool reason_ok = false;
    for (int i = 0; valid_reasons[i] != NULL; i++) {
        if (strcmp(reason, valid_reasons[i]) == 0) { reason_ok = true; break; }
    }
    if (!reason_ok) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                            "reason must be: defective, wrong_item, changed_mind, or other");
        return;
    }

    char *c_order_id    = str_dup(order_id);
    char *c_item_id     = (item_id && !str_is_empty(item_id)) ? str_dup(item_id) : NULL;
    char *c_reason      = str_dup(reason);
    char *c_description = (description && !str_is_empty(description)) ? str_dup(description) : NULL;
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(c_order_id); free(c_item_id); free(c_reason); free(c_description);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Verify the order belongs to this user and is delivered */
    const char *order_query =
        "SELECT o.id, o.status FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *order_params[] = { c_order_id, user_id };
    PGresult *order_result = db_exec_params(conn, order_query, 2, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        free(c_order_id); free(c_item_id); free(c_reason); free(c_description);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    DbRow order_row = { .result = order_result, .row = 0 };
    const char *order_status = db_row_get_string(&order_row, "status");

    if (strcmp(order_status, "delivered") != 0) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        free(c_order_id); free(c_item_id); free(c_reason); free(c_description);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                            "Returns can only be requested for delivered orders");
        return;
    }
    PQclear(order_result);

    /* Check if a return for this order already exists */
    const char *existing_query =
        "SELECT id FROM order_returns WHERE order_id = $1 "
        "AND status NOT IN ('rejected', 'refunded') LIMIT 1";
    const char *existing_params[] = { c_order_id };
    PGresult *existing_result = db_exec_params(conn, existing_query, 1, existing_params);

    if (db_result_ok(existing_result) && db_result_has_rows(existing_result)) {
        PQclear(existing_result);
        db_pool_release(pool, conn);
        free(c_order_id); free(c_item_id); free(c_reason); free(c_description);
        http_response_error(resp, HTTP_STATUS_CONFLICT,
                            "A return request for this order is already in progress");
        return;
    }
    PQclear(existing_result);

    /* Insert return */
    const char *insert_query =
        "INSERT INTO order_returns (order_id, order_item_id, reason, description) "
        "VALUES ($1, $2, $3, $4) RETURNING id, created_at";
    const char *insert_params[] = { c_order_id, c_item_id, c_reason, c_description };

    PGresult *insert_result = db_exec_params(conn, insert_query, 4, insert_params);
    db_pool_release(pool, conn);

    if (!db_result_ok(insert_result) || !db_result_has_rows(insert_result)) {
        PQclear(insert_result);
        free(c_order_id); free(c_item_id); free(c_reason); free(c_description);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create return request");
        return;
    }

    DbRow ret_row = { .result = insert_result, .row = 0 };
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", db_row_get_string(&ret_row, "id"));
    cJSON_AddStringToObject(data, "order_id", c_order_id);
    cJSON_AddStringToObject(data, "reason", c_reason);
    cJSON_AddStringToObject(data, "status", "requested");
    cJSON_AddStringToObject(data, "created_at", db_row_get_string(&ret_row, "created_at"));
    PQclear(insert_result);

    LOG_INFO("Return request created for order %s", c_order_id);
    free(c_order_id); free(c_item_id); free(c_reason); free(c_description);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

void handler_returns_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT r.id, r.order_id, o.order_number, r.reason, r.description, "
        "r.status, r.refund_amount, r.created_at "
        "FROM order_returns r "
        "JOIN orders o ON r.order_id = o.id "
        "JOIN users u ON o.user_id = u.id "
        "WHERE u.id = $1 OR u.supabase_id = $1 "
        "ORDER BY r.created_at DESC";
    const char *params[] = { user_id };

    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *returns = db_result_to_json(result);
    PQclear(result);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "returns", returns);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_returns_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *return_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (return_id == NULL || !uuid_validate(return_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid return ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT r.id, r.order_id, o.order_number, r.order_item_id, "
        "r.reason, r.description, r.status, r.admin_notes, r.refund_amount, "
        "r.created_at, r.resolved_at "
        "FROM order_returns r "
        "JOIN orders o ON r.order_id = o.id "
        "JOIN users u ON o.user_id = u.id "
        "WHERE r.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *params[] = { return_id, user_id };

    PGresult *result = db_exec_params(conn, query, 2, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Return request not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* ---- Admin handlers ---- */

void handler_admin_returns_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    const char *status_filter = http_request_get_query_param(req, "status");
    const char *page_str  = http_request_get_query_param(req, "page");
    const char *limit_str = http_request_get_query_param(req, "limit");

    int page  = page_str  ? atoi(page_str)  : 1;
    int limit = limit_str ? atoi(limit_str) : 20;
    if (page < 1) page = 1;
    if (limit < 1 || limit > 100) limit = 20;

    int offset = (page - 1) * limit;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build query with optional status filter */
    char where[256] = "1=1";
    const char *filter_params[3];
    int param_count = 0;

    if (status_filter != NULL && status_filter[0] != '\0') {
        param_count++;
        filter_params[param_count - 1] = status_filter;
        snprintf(where, sizeof(where), "r.status = $%d", param_count);
    }

    char limit_s[16], offset_s[16];
    snprintf(limit_s, sizeof(limit_s), "%d", limit);
    snprintf(offset_s, sizeof(offset_s), "%d", offset);
    filter_params[param_count]     = limit_s;
    filter_params[param_count + 1] = offset_s;

    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT r.id, r.order_id, o.order_number, r.reason, r.description, "
        "r.status, r.refund_amount, r.admin_notes, r.created_at, r.resolved_at, "
        "u.email AS customer_email, u.full_name AS customer_name "
        "FROM order_returns r "
        "JOIN orders o ON r.order_id = o.id "
        "LEFT JOIN users u ON o.user_id = u.id "
        "WHERE %s "
        "ORDER BY r.created_at DESC "
        "LIMIT $%d OFFSET $%d",
        where, param_count + 1, param_count + 2);

    PGresult *result = db_exec_params(conn, query, param_count + 2, filter_params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *returns = db_result_to_json(result);
    PQclear(result);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "returns", returns);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/*
 * PUT /api/admin/returns/:id/status
 * Body: { "status": "approved|rejected|refunded|under_review",
 *         "admin_notes": "...", "refund_amount": 99.90 }
 */
void handler_admin_returns_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *return_id = http_request_get_path_param(req, "id");

    if (return_id == NULL || !uuid_validate(return_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid return ID");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *status      = json_get_string(body, "status", NULL);
    const char *admin_notes = json_get_string(body, "admin_notes", NULL);
    double refund_amount    = 0;

    cJSON *refund_json = cJSON_GetObjectItem(body, "refund_amount");
    if (refund_json && cJSON_IsNumber(refund_json)) {
        refund_amount = cJSON_GetNumberValue(refund_json);
    }

    if (str_is_empty(status)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "status is required");
        return;
    }

    const char *valid_statuses[] = {
        "under_review", "approved", "rejected", "refunded", NULL
    };
    bool status_ok = false;
    for (int i = 0; valid_statuses[i] != NULL; i++) {
        if (strcmp(status, valid_statuses[i]) == 0) { status_ok = true; break; }
    }
    if (!status_ok) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                            "status must be: under_review, approved, rejected, or refunded");
        return;
    }

    char *c_status      = str_dup(status);
    char *c_notes       = (admin_notes && !str_is_empty(admin_notes)) ? str_dup(admin_notes) : NULL;
    char refund_s[32];
    snprintf(refund_s, sizeof(refund_s), "%.2f", refund_amount);
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(c_status); free(c_notes);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *resolved_clause =
        (strcmp(c_status, "approved") == 0 || strcmp(c_status, "rejected") == 0 ||
         strcmp(c_status, "refunded") == 0)
        ? ", resolved_at = NOW()" : "";

    char update_query[512];
    snprintf(update_query, sizeof(update_query),
        "UPDATE order_returns SET status = $1, admin_notes = COALESCE($2, admin_notes), "
        "refund_amount = CASE WHEN $3::numeric > 0 THEN $3::numeric ELSE refund_amount END%s "
        "WHERE id = $4 RETURNING id, status, admin_notes, refund_amount, resolved_at",
        resolved_clause);

    const char *params[] = { c_status, c_notes, refund_s, return_id };
    PGresult *result = db_exec_params(conn, update_query, 4, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        free(c_status); free(c_notes);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Return request not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);

    LOG_INFO("Return %s updated to status: %s", return_id, c_status);
    free(c_status); free(c_notes);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
