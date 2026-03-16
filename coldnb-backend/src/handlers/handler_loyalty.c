#include "handlers/handler_loyalty.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Points earned per R$ spent (1 point per R$ 1) */
#define POINTS_PER_REAL 1

void handler_loyalty_register(HttpRouter *router, DbPool *pool) {
    /* Customer routes (auth required, middleware applied via /api/loyalty path) */
    ROUTE_GET(router, "/api/loyalty/balance", handler_loyalty_balance, pool);
    ROUTE_GET(router, "/api/loyalty/history", handler_loyalty_history, pool);
    ROUTE_GET(router, "/api/loyalty/rewards", handler_loyalty_rewards_list, pool);
    ROUTE_POST(router, "/api/loyalty/redeem", handler_loyalty_redeem, pool);

    /* Admin routes */
    ROUTE_GET(router, "/api/admin/loyalty/rewards", handler_admin_loyalty_rewards_list, pool);
    ROUTE_POST(router, "/api/admin/loyalty/rewards", handler_admin_loyalty_rewards_create, pool);
    ROUTE_PUT(router, "/api/admin/loyalty/rewards/:id", handler_admin_loyalty_rewards_update, pool);
    ROUTE_DELETE(router, "/api/admin/loyalty/rewards/:id", handler_admin_loyalty_rewards_delete, pool);
    ROUTE_POST(router, "/api/admin/loyalty/grant", handler_admin_loyalty_grant, pool);
}

void handler_loyalty_balance(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    const char *query = "SELECT COALESCE(SUM(points), 0) AS balance FROM loyalty_points WHERE user_id = $1";
    const char *params[] = { user_id };
    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int balance = 0;
    if (PQntuples(result) > 0) {
        DbRow row = { .result = result, .row = 0 };
        balance = db_row_get_int(&row, "balance");
    }
    PQclear(result);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "balance", balance);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_loyalty_history(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        "SELECT id, points, reason, reference_id, created_at "
        "FROM loyalty_points WHERE user_id = $1 "
        "ORDER BY created_at DESC LIMIT 50";
    const char *params[] = { user_id };
    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < rows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", db_row_get_string(&row, "id"));
        cJSON_AddNumberToObject(item, "points", db_row_get_int(&row, "points"));
        cJSON_AddStringToObject(item, "reason", db_row_get_string(&row, "reason"));

        const char *ref = db_row_get_string(&row, "reference_id");
        if (!str_is_empty(ref)) {
            cJSON_AddStringToObject(item, "reference_id", ref);
        } else {
            cJSON_AddNullToObject(item, "reference_id");
        }

        cJSON_AddStringToObject(item, "created_at", db_row_get_string(&row, "created_at"));
        cJSON_AddItemToArray(arr, item);
    }

    PQclear(result);

    cJSON *response = json_create_success(arr);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_loyalty_rewards_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    (void)req;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT id, name, description, points_cost, reward_type, reward_value "
        "FROM loyalty_rewards WHERE is_active = true "
        "ORDER BY points_cost ASC";
    PGresult *result = db_exec_params(conn, query, 0, NULL);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < rows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", db_row_get_int(&row, "id"));
        cJSON_AddStringToObject(item, "name", db_row_get_string(&row, "name"));

        const char *desc = db_row_get_string(&row, "description");
        if (!str_is_empty(desc)) {
            cJSON_AddStringToObject(item, "description", desc);
        } else {
            cJSON_AddNullToObject(item, "description");
        }

        cJSON_AddNumberToObject(item, "points_cost", db_row_get_int(&row, "points_cost"));
        cJSON_AddStringToObject(item, "reward_type", db_row_get_string(&row, "reward_type"));
        cJSON_AddNumberToObject(item, "reward_value", db_row_get_double(&row, "reward_value"));
        cJSON_AddItemToArray(arr, item);
    }

    PQclear(result);

    cJSON *response = json_create_success(arr);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_loyalty_redeem(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    int reward_id = json_get_int(body, "reward_id", 0);
    cJSON_Delete(body);

    if (reward_id <= 0) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "reward_id is required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get reward details */
    char reward_id_str[16];
    snprintf(reward_id_str, sizeof(reward_id_str), "%d", reward_id);
    const char *reward_query =
        "SELECT id, name, points_cost, reward_type, reward_value "
        "FROM loyalty_rewards WHERE id = $1 AND is_active = true";
    const char *rp[] = { reward_id_str };
    PGresult *reward_result = db_exec_params(conn, reward_query, 1, rp);

    if (!db_result_ok(reward_result) || PQntuples(reward_result) == 0) {
        PQclear(reward_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Reward not found");
        return;
    }

    DbRow rrow = { .result = reward_result, .row = 0 };
    int points_cost = db_row_get_int(&rrow, "points_cost");
    double reward_value = db_row_get_double(&rrow, "reward_value");
    char *reward_name = str_dup(db_row_get_string(&rrow, "name"));
    char *reward_type = str_dup(db_row_get_string(&rrow, "reward_type"));
    PQclear(reward_result);

    /* Check user balance */
    const char *balance_query = "SELECT COALESCE(SUM(points), 0) AS balance FROM loyalty_points WHERE user_id = $1";
    const char *bp[] = { user_id };
    PGresult *balance_result = db_exec_params(conn, balance_query, 1, bp);

    if (!db_result_ok(balance_result)) {
        PQclear(balance_result);
        db_pool_release(pool, conn);
        free(reward_name);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Balance check failed");
        return;
    }

    int balance = 0;
    if (PQntuples(balance_result) > 0) {
        DbRow brow = { .result = balance_result, .row = 0 };
        balance = db_row_get_int(&brow, "balance");
    }
    PQclear(balance_result);

    if (balance < points_cost) {
        db_pool_release(pool, conn);
        free(reward_name);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Insufficient points");
        return;
    }

    /* Generate a discount code for the reward */
    char *random_suffix = str_random(6);
    char *discount_code = str_printf("LOYALTY-%s", random_suffix ? random_suffix : "REWARD");
    free(random_suffix);
    str_to_upper(discount_code);

    /* Determine discount type based on reward_type */
    const char *discount_type = "fixed";
    if (reward_type != NULL && strcmp(reward_type, "percentage") == 0) {
        discount_type = "percentage";
    }

    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%.2f", reward_value);

    /* Create the discount code */
    const char *create_code_query =
        "INSERT INTO discount_codes (code, description, discount_type, discount_value, "
        "usage_limit, is_active) "
        "VALUES ($1, $2, $3, $4::numeric, 1, true) RETURNING id";
    char *desc = str_printf("Loyalty reward: %s", reward_name ? reward_name : "Reward");
    const char *ccp[] = { discount_code, desc, discount_type, value_str };
    PGresult *code_result = db_exec_params(conn, create_code_query, 4, ccp);

    if (!db_result_ok(code_result) || PQntuples(code_result) == 0) {
        PQclear(code_result);
        db_pool_release(pool, conn);
        free(reward_name);
        free(reward_type);
        free(discount_code);
        free(desc);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create discount code");
        return;
    }
    PQclear(code_result);
    free(desc);

    /* Deduct points */
    char points_str[16];
    snprintf(points_str, sizeof(points_str), "%d", -points_cost);
    char *reason = str_printf("Redeemed: %s", reward_name ? reward_name : "Reward");
    const char *deduct_query =
        "INSERT INTO loyalty_points (user_id, points, reason, reference_id) "
        "VALUES ($1, $2::int, $3, $4)";
    const char *dp[] = { user_id, points_str, reason, discount_code };
    PGresult *deduct_result = db_exec_params(conn, deduct_query, 4, dp);
    PQclear(deduct_result);
    free(reason);

    /* Record redemption */
    const char *redeem_query =
        "INSERT INTO loyalty_redemptions (user_id, reward_id, points_spent, discount_code) "
        "VALUES ($1, $2::int, $3::int, $4)";
    char cost_str[16];
    snprintf(cost_str, sizeof(cost_str), "%d", points_cost);
    const char *rdp[] = { user_id, reward_id_str, cost_str, discount_code };
    PGresult *redeem_result = db_exec_params(conn, redeem_query, 4, rdp);
    PQclear(redeem_result);

    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "discount_code", discount_code);
    cJSON_AddNumberToObject(data, "points_spent", points_cost);
    cJSON_AddNumberToObject(data, "new_balance", balance - points_cost);
    cJSON_AddStringToObject(data, "reward_name", reward_name ? reward_name : "Reward");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Loyalty redemption: user=%s reward=%s code=%s", user_id, reward_name ? reward_name : "?", discount_code);

    free(reward_name);
    free(reward_type);
    free(discount_code);
}

/* ---- Admin handlers ---- */

void handler_admin_loyalty_rewards_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    (void)req;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT id, name, description, points_cost, reward_type, reward_value, is_active, created_at "
        "FROM loyalty_rewards ORDER BY created_at DESC";
    PGresult *result = db_exec_params(conn, query, 0, NULL);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < rows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", db_row_get_int(&row, "id"));
        cJSON_AddStringToObject(item, "name", db_row_get_string(&row, "name"));

        const char *d = db_row_get_string(&row, "description");
        if (!str_is_empty(d)) {
            cJSON_AddStringToObject(item, "description", d);
        } else {
            cJSON_AddNullToObject(item, "description");
        }

        cJSON_AddNumberToObject(item, "points_cost", db_row_get_int(&row, "points_cost"));
        cJSON_AddStringToObject(item, "reward_type", db_row_get_string(&row, "reward_type"));
        cJSON_AddNumberToObject(item, "reward_value", db_row_get_double(&row, "reward_value"));
        cJSON_AddBoolToObject(item, "is_active", db_row_get_bool(&row, "is_active"));
        cJSON_AddStringToObject(item, "created_at", db_row_get_string(&row, "created_at"));
        cJSON_AddItemToArray(arr, item);
    }

    PQclear(result);

    cJSON *response = json_create_success(arr);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_loyalty_rewards_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    char *name = str_dup(json_get_string(body, "name", NULL));
    char *description = str_dup(json_get_string(body, "description", NULL));
    int points_cost = json_get_int(body, "points_cost", 0);
    char *reward_type = str_dup(json_get_string(body, "reward_type", "discount"));
    double reward_value = json_get_double(body, "reward_value", 0);
    cJSON_Delete(body);

    if (str_is_empty(name) || points_cost <= 0 || reward_value <= 0) {
        free(name);
        free(description);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "name, points_cost, and reward_value are required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(name);
        free(description);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char cost_str[16];
    snprintf(cost_str, sizeof(cost_str), "%d", points_cost);
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%.2f", reward_value);

    const char *query =
        "INSERT INTO loyalty_rewards (name, description, points_cost, reward_type, reward_value) "
        "VALUES ($1, $2, $3::int, $4, $5::numeric) "
        "RETURNING id, name, description, points_cost, reward_type, reward_value, is_active, created_at";
    const char *params[] = { name, description, cost_str, reward_type, value_str };
    PGresult *result = db_exec_params(conn, query, 5, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result) || PQntuples(result) == 0) {
        PQclear(result);
        free(name);
        free(description);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create reward");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", db_row_get_int(&row, "id"));
    cJSON_AddStringToObject(data, "name", db_row_get_string(&row, "name"));
    cJSON_AddNumberToObject(data, "points_cost", db_row_get_int(&row, "points_cost"));
    cJSON_AddStringToObject(data, "reward_type", db_row_get_string(&row, "reward_type"));
    cJSON_AddNumberToObject(data, "reward_value", db_row_get_double(&row, "reward_value"));
    cJSON_AddBoolToObject(data, "is_active", db_row_get_bool(&row, "is_active"));

    PQclear(result);
    free(name);
    free(description);
    free(reward_type);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

void handler_admin_loyalty_rewards_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (str_is_empty(id_str)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Reward ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    char *name = str_dup(json_get_string(body, "name", NULL));
    char *description = str_dup(json_get_string(body, "description", NULL));
    int points_cost = json_get_int(body, "points_cost", 0);
    char *reward_type = str_dup(json_get_string(body, "reward_type", NULL));
    double reward_value = json_get_double(body, "reward_value", 0);
    cJSON *is_active_item = cJSON_GetObjectItem(body, "is_active");
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(name);
        free(description);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build dynamic update */
    char set_clause[512] = "";
    const char *params[8];
    int param_count = 0;

    if (!str_is_empty(name)) {
        params[param_count++] = name;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "name = $%d, ", param_count);
    }
    if (description != NULL) {
        params[param_count++] = description;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "description = $%d, ", param_count);
    }

    char cost_str[16] = "";
    if (points_cost > 0) {
        snprintf(cost_str, sizeof(cost_str), "%d", points_cost);
        params[param_count++] = cost_str;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "points_cost = $%d::int, ", param_count);
    }

    if (!str_is_empty(reward_type)) {
        params[param_count++] = reward_type;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "reward_type = $%d, ", param_count);
    }

    char value_str[32] = "";
    if (reward_value > 0) {
        snprintf(value_str, sizeof(value_str), "%.2f", reward_value);
        params[param_count++] = value_str;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "reward_value = $%d::numeric, ", param_count);
    }

    char active_str[8] = "";
    if (is_active_item != NULL && cJSON_IsBool(is_active_item)) {
        snprintf(active_str, sizeof(active_str), "%s", cJSON_IsTrue(is_active_item) ? "true" : "false");
        params[param_count++] = active_str;
        snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
                 "is_active = $%d::bool, ", param_count);
    }

    if (param_count == 0) {
        db_pool_release(pool, conn);
        free(name);
        free(description);
        free(reward_type);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "No fields to update");
        return;
    }

    /* Remove trailing comma-space */
    size_t len = strlen(set_clause);
    if (len >= 2) set_clause[len - 2] = '\0';

    /* Add id as last param */
    params[param_count++] = id_str;

    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE loyalty_rewards SET %s WHERE id = $%d "
             "RETURNING id, name, description, points_cost, reward_type, reward_value, is_active",
             set_clause, param_count);

    PGresult *result = db_exec_params(conn, query, param_count, params);
    db_pool_release(pool, conn);

    free(name);
    free(description);
    free(reward_type);

    if (!db_result_ok(result) || PQntuples(result) == 0) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Reward not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", db_row_get_int(&row, "id"));
    cJSON_AddStringToObject(data, "name", db_row_get_string(&row, "name"));
    cJSON_AddNumberToObject(data, "points_cost", db_row_get_int(&row, "points_cost"));
    cJSON_AddStringToObject(data, "reward_type", db_row_get_string(&row, "reward_type"));
    cJSON_AddNumberToObject(data, "reward_value", db_row_get_double(&row, "reward_value"));
    cJSON_AddBoolToObject(data, "is_active", db_row_get_bool(&row, "is_active"));

    PQclear(result);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_loyalty_rewards_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (str_is_empty(id_str)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Reward ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query = "DELETE FROM loyalty_rewards WHERE id = $1";
    const char *params[] = { id_str };
    PGresult *result = db_exec_params(conn, query, 1, params);
    db_pool_release(pool, conn);

    bool deleted = db_result_ok(result) && atoi(PQcmdTuples(result)) > 0;
    PQclear(result);

    if (!deleted) {
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Reward not found");
        return;
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", "Reward deleted");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_loyalty_grant(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    char *user_id = str_dup(json_get_string(body, "user_id", NULL));
    int points = json_get_int(body, "points", 0);
    char *reason = str_dup(json_get_string(body, "reason", "Admin grant"));
    cJSON_Delete(body);

    if (str_is_empty(user_id) || points == 0) {
        free(user_id);
        free(reason);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "user_id and non-zero points are required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(user_id);
        free(reason);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char points_str[16];
    snprintf(points_str, sizeof(points_str), "%d", points);

    const char *query =
        "INSERT INTO loyalty_points (user_id, points, reason) "
        "VALUES ($1, $2::int, $3) RETURNING id";
    const char *params[] = { user_id, points_str, reason };
    PGresult *result = db_exec_params(conn, query, 3, params);
    db_pool_release(pool, conn);

    if (!db_result_ok(result) || PQntuples(result) == 0) {
        PQclear(result);
        free(user_id);
        free(reason);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to grant points");
        return;
    }

    PQclear(result);

    LOG_INFO("Loyalty points granted: user=%s points=%d reason=%s", user_id, points, reason);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", "Points granted successfully");
    cJSON_AddNumberToObject(data, "points", points);

    free(user_id);
    free(reason);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
