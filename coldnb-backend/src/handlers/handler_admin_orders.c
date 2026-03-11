#include "handlers/handler_admin_orders.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "services/svc_email.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

/* Valid order statuses */
static const char *VALID_STATUSES[] = {
    "pending", "confirmed", "processing", "shipped", "delivered", "cancelled", NULL
};

static bool is_valid_status(const char *status) {
    for (int i = 0; VALID_STATUSES[i] != NULL; i++) {
        if (strcmp(status, VALID_STATUSES[i]) == 0) {
            return true;
        }
    }
    return false;
}

void handler_admin_orders_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/orders", handler_admin_orders_list, pool);
    ROUTE_GET(router, "/api/admin/orders/:id", handler_admin_orders_get, pool);
    ROUTE_PUT(router, "/api/admin/orders/:id/status", handler_admin_orders_update_status, pool);

    ROUTE_GET(router, "/api/admin/users", handler_admin_users_list, pool);
    ROUTE_GET(router, "/api/admin/users/:id", handler_admin_users_get, pool);
    ROUTE_PUT(router, "/api/admin/users/:id", handler_admin_users_update, pool);
}

void handler_admin_orders_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    /* Pagination and filters */
    const char *page_str = http_request_get_query_param(req, "page");
    const char *limit_str = http_request_get_query_param(req, "limit");
    const char *status = http_request_get_query_param(req, "status");
    const char *search = http_request_get_query_param(req, "search");

    int page = page_str ? atoi(page_str) : 1;
    int limit = limit_str ? atoi(limit_str) : 20;

    if (page < 1) page = 1;
    if (limit < 1) limit = 20;
    if (limit > 100) limit = 100;

    int offset = (page - 1) * limit;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build count query */
    char count_query[1024];
    snprintf(count_query, sizeof(count_query),
             "SELECT COUNT(*) FROM orders o "
             "LEFT JOIN users u ON o.user_id = u.id "
             "WHERE 1=1 %s %s",
             status ? "AND o.status = $1" : "",
             search ? (status ? "AND (o.order_number ILIKE $2 OR u.email ILIKE $2)"
                              : "AND (o.order_number ILIKE $1 OR u.email ILIKE $1)") : "");

    int param_count = 0;
    const char *count_params[2];

    if (status) {
        count_params[param_count++] = status;
    }
    if (search) {
        char *search_pattern = str_printf("%%%s%%", search);
        count_params[param_count++] = search_pattern;
        int total = db_count(conn, count_query, param_count, count_params);
        free(search_pattern);

        /* Re-setup for main query */
        param_count = 0;
        if (status) count_params[param_count++] = status;
        search_pattern = str_printf("%%%s%%", search);
        count_params[param_count] = search_pattern;

        /* Main query */
        char limit_str_buf[16], offset_str_buf[16];
        snprintf(limit_str_buf, sizeof(limit_str_buf), "%d", limit);
        snprintf(offset_str_buf, sizeof(offset_str_buf), "%d", offset);

        char query[2048];
        snprintf(query, sizeof(query),
                 "SELECT o.id, o.order_number, o.status, o.payment_status, o.payment_method, "
                 "o.total, o.created_at, u.email AS customer_email, u.full_name AS customer_name, "
                 "(SELECT COUNT(*) FROM order_items WHERE order_id = o.id) AS item_count "
                 "FROM orders o "
                 "LEFT JOIN users u ON o.user_id = u.id "
                 "WHERE 1=1 %s %s "
                 "ORDER BY o.created_at DESC "
                 "LIMIT %s OFFSET %s",
                 status ? "AND o.status = $1" : "",
                 search ? (status ? "AND (o.order_number ILIKE $2 OR u.email ILIKE $2)"
                                  : "AND (o.order_number ILIKE $1 OR u.email ILIKE $1)") : "",
                 limit_str_buf, offset_str_buf);

        PGresult *result = db_exec_params(conn, query, param_count + 1, count_params);
        free(search_pattern);

        if (!db_result_ok(result)) {
            PQclear(result);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
            return;
        }

        cJSON *orders = db_result_to_json(result);
        PQclear(result);
        db_pool_release(pool, conn);

        int total_pages = (total + limit - 1) / limit;

        cJSON *data = cJSON_CreateObject();
        cJSON_AddItemToObject(data, "orders", orders);
        cJSON_AddItemToObject(data, "pagination",
                             json_create_pagination(page, limit, total, total_pages));

        cJSON *response = json_create_success(data);
        char *json = cJSON_PrintUnformatted(response);
        cJSON_Delete(response);

        http_response_json(resp, HTTP_STATUS_OK, json);
        free(json);
        return;
    }

    int total = db_count(conn, count_query, param_count, count_params);

    /* Main query */
    char limit_str_buf[16], offset_str_buf[16];
    snprintf(limit_str_buf, sizeof(limit_str_buf), "%d", limit);
    snprintf(offset_str_buf, sizeof(offset_str_buf), "%d", offset);

    char query[2048];
    snprintf(query, sizeof(query),
             "SELECT o.id, o.order_number, o.status, o.payment_status, o.payment_method, "
             "o.total, o.created_at, u.email AS customer_email, u.full_name AS customer_name, "
             "(SELECT COUNT(*) FROM order_items WHERE order_id = o.id) AS item_count "
             "FROM orders o "
             "LEFT JOIN users u ON o.user_id = u.id "
             "WHERE 1=1 %s "
             "ORDER BY o.created_at DESC "
             "LIMIT %s OFFSET %s",
             status ? "AND o.status = $1" : "",
             limit_str_buf, offset_str_buf);

    PGresult *result = db_exec_params(conn, query, param_count, count_params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *orders = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    int total_pages = (total + limit - 1) / limit;

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "orders", orders);
    cJSON_AddItemToObject(data, "pagination",
                         json_create_pagination(page, limit, total, total_pages));

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_orders_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *order_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (order_id == NULL || !uuid_validate(order_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid order ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get order details */
    const char *order_query =
        "SELECT o.*, u.email AS customer_email, u.full_name AS customer_name, u.phone AS customer_phone "
        "FROM orders o "
        "LEFT JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1";
    const char *order_params[] = { order_id };

    PGresult *order_result = db_exec_params(conn, order_query, 1, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    DbRow order_row = { .result = order_result, .row = 0 };
    cJSON *data = db_row_to_json(&order_row);
    PQclear(order_result);

    /* Get order items */
    const char *items_query =
        "SELECT * FROM order_items WHERE order_id = $1 ORDER BY id";
    PGresult *items_result = db_exec_params(conn, items_query, 1, order_params);

    if (db_result_ok(items_result)) {
        cJSON_AddItemToObject(data, "items", db_result_to_json(items_result));
    }
    PQclear(items_result);

    /* Get order history */
    const char *history_query =
        "SELECT oh.*, au.username AS admin_username "
        "FROM order_history oh "
        "LEFT JOIN admin_users au ON oh.created_by = au.id "
        "WHERE oh.order_id = $1 "
        "ORDER BY oh.created_at DESC";
    PGresult *history_result = db_exec_params(conn, history_query, 1, order_params);

    if (db_result_ok(history_result)) {
        cJSON_AddItemToObject(data, "history", db_result_to_json(history_result));
    }
    PQclear(history_result);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_orders_update_status(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *order_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (order_id == NULL || !uuid_validate(order_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid order ID");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *status = json_get_string(body, "status", NULL);
    const char *notes = json_get_string(body, "notes", NULL);
    char *status_copy = NULL;
    char *notes_copy = NULL;

    if (str_is_empty(status)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Status required");
        return;
    }

    if (!is_valid_status(status)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid status");
        return;
    }

    status_copy = str_dup(status);
    notes_copy = notes ? str_dup(notes) : NULL;

    AuthContext *auth = auth_get_context(req);
    const char *admin_id = auth ? auth->user_id : NULL;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        free(status_copy);
        free(notes_copy);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    if (!db_begin(conn)) {
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        free(status_copy);
        free(notes_copy);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* Update order status */
    const char *update_query =
        "UPDATE orders SET status = $1, updated_at = NOW() "
        "WHERE id = $2 RETURNING order_number";
    const char *update_params[] = { status_copy, order_id };

    PGresult *update_result = db_exec_params(conn, update_query, 2, update_params);

    if (!db_result_ok(update_result) || !db_result_has_rows(update_result)) {
        PQclear(update_result);
        db_rollback(conn);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        free(status_copy);
        free(notes_copy);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    const char *order_number = db_result_value(update_result);
    char *order_num_copy = str_dup(order_number);
    PQclear(update_result);

    const char *notify_query =
        "SELECT u.email, u.full_name "
        "FROM orders o "
        "LEFT JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1";
    const char *notify_params[] = { order_id };
    PGresult *notify_result = db_exec_params(conn, notify_query, 1, notify_params);
    char *customer_email = NULL;
    char *customer_name = NULL;
    if (db_result_ok(notify_result) && db_result_has_rows(notify_result)) {
        DbRow notify_row = { .result = notify_result, .row = 0 };
        const char *email_value = db_row_get_string(&notify_row, "email");
        const char *name_value = db_row_get_string(&notify_row, "full_name");
        customer_email = email_value ? str_dup(email_value) : NULL;
        customer_name = name_value ? str_dup(name_value) : NULL;
    }
    PQclear(notify_result);

    /* Update timestamp fields based on status */
    if (strcmp(status_copy, "shipped") == 0) {
        const char *ship_query = "UPDATE orders SET shipped_at = NOW() WHERE id = $1";
        const char *ship_params[] = { order_id };
        PGresult *ship_result = db_exec_params(conn, ship_query, 1, ship_params);
        PQclear(ship_result);
    } else if (strcmp(status_copy, "delivered") == 0) {
        const char *deliver_query = "UPDATE orders SET delivered_at = NOW() WHERE id = $1";
        const char *deliver_params[] = { order_id };
        PGresult *deliver_result = db_exec_params(conn, deliver_query, 1, deliver_params);
        PQclear(deliver_result);
    } else if (strcmp(status_copy, "cancelled") == 0) {
        const char *cancel_query = "UPDATE orders SET cancelled_at = NOW() WHERE id = $1";
        const char *cancel_params[] = { order_id };
        PGresult *cancel_result = db_exec_params(conn, cancel_query, 1, cancel_params);
        PQclear(cancel_result);
    }

    /* Add history entry */
    const char *history_query =
        "INSERT INTO order_history (order_id, status, notes, created_by) "
        "VALUES ($1, $2, $3, $4)";
    const char *history_params[] = { order_id, status_copy, notes_copy, admin_id };
    PGresult *history_result = db_exec_params(conn, history_query, 4, history_params);
    PQclear(history_result);

    if (!db_commit(conn)) {
        db_rollback(conn);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        free(order_num_copy);
        free(customer_email);
        free(customer_name);
        free(status_copy);
        free(notes_copy);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to update order status");
        return;
    }
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "order_number", order_num_copy);
    cJSON_AddStringToObject(data, "status", status_copy);
    cJSON_AddStringToObject(data, "message", "Order status updated");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    EmailOrderStatusUpdate email_update = {
        .order_number = order_num_copy,
        .customer_email = customer_email,
        .customer_name = customer_name,
        .status = status_copy
    };
    if (email_service_send_order_status_update(&email_update) != 0) {
        LOG_WARN("Order status email failed for order %s", order_num_copy);
    }

    LOG_INFO("Order %s status updated to: %s", order_id, status_copy);
    free(order_num_copy);
    free(customer_email);
    free(customer_name);
    free(status_copy);
    free(notes_copy);
}

void handler_admin_users_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *page_str = http_request_get_query_param(req, "page");
    const char *limit_str = http_request_get_query_param(req, "limit");
    const char *search = http_request_get_query_param(req, "search");

    int page = page_str ? atoi(page_str) : 1;
    int limit = limit_str ? atoi(limit_str) : 20;

    if (page < 1) page = 1;
    if (limit < 1) limit = 20;
    if (limit > 100) limit = 100;

    int offset = (page - 1) * limit;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char limit_str_buf[16], offset_str_buf[16];
    snprintf(limit_str_buf, sizeof(limit_str_buf), "%d", limit);
    snprintf(offset_str_buf, sizeof(offset_str_buf), "%d", offset);

    int total;
    cJSON *users;

    if (search && search[0] != '\0') {
        char *search_pattern = str_printf("%%%s%%", search);

        const char *count_query =
            "SELECT COUNT(*) FROM users WHERE email ILIKE $1 OR full_name ILIKE $1";
        const char *count_params[] = { search_pattern };
        total = db_count(conn, count_query, 1, count_params);

        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT id, email, full_name, phone, is_active, email_verified, created_at "
                 "FROM users WHERE email ILIKE $1 OR full_name ILIKE $1 "
                 "ORDER BY created_at DESC LIMIT %s OFFSET %s",
                 limit_str_buf, offset_str_buf);

        PGresult *result = db_exec_params(conn, query, 1, count_params);
        free(search_pattern);

        if (!db_result_ok(result)) {
            PQclear(result);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
            return;
        }

        users = db_result_to_json(result);
        PQclear(result);
    } else {
        const char *count_query = "SELECT COUNT(*) FROM users";
        total = db_count(conn, count_query, 0, NULL);

        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT id, email, full_name, phone, is_active, email_verified, created_at "
                 "FROM users ORDER BY created_at DESC LIMIT %s OFFSET %s",
                 limit_str_buf, offset_str_buf);

        PGresult *result = db_exec(conn, query);

        if (!db_result_ok(result)) {
            PQclear(result);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
            return;
        }

        users = db_result_to_json(result);
        PQclear(result);
    }

    db_pool_release(pool, conn);

    int total_pages = (total + limit - 1) / limit;

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "users", users);
    cJSON_AddItemToObject(data, "pagination",
                         json_create_pagination(page, limit, total, total_pages));

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_users_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (user_id == NULL || !uuid_validate(user_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid user ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *user_query =
        "SELECT id, email, full_name, phone, avatar_url, is_active, email_verified, created_at "
        "FROM users WHERE id = $1";
    const char *user_params[] = { user_id };

    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    DbRow row = { .result = user_result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(user_result);

    /* Get order stats */
    const char *stats_query =
        "SELECT COUNT(*) AS order_count, "
        "COALESCE(SUM(total), 0) AS total_spent "
        "FROM orders WHERE user_id = $1";
    PGresult *stats_result = db_exec_params(conn, stats_query, 1, user_params);

    if (db_result_ok(stats_result) && db_result_has_rows(stats_result)) {
        DbRow stats_row = { .result = stats_result, .row = 0 };
        cJSON_AddNumberToObject(data, "order_count", db_row_get_int(&stats_row, "order_count"));
        cJSON_AddNumberToObject(data, "total_spent", db_row_get_double(&stats_row, "total_spent"));
    }
    PQclear(stats_result);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_users_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (user_id == NULL || !uuid_validate(user_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid user ID");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Only allow updating certain fields */
    const char *update_query =
        "UPDATE users SET "
        "full_name = COALESCE($2, full_name), "
        "phone = COALESCE($3, phone), "
        "is_active = COALESCE($4, is_active), "
        "updated_at = NOW() "
        "WHERE id = $1 "
        "RETURNING id, email, full_name, phone, is_active";

    const char *is_active_str = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        user_id,
        json_get_string(body, "full_name", NULL),
        json_get_string(body, "phone", NULL),
        is_active_str
    };

    PGresult *result = db_exec_params(conn, update_query, 4, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("User updated by admin: %s", user_id);
}
