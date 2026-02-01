#include "handlers/handler_wishlist.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_wishlist_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/wishlist", handler_wishlist_list, pool);
    ROUTE_POST(router, "/api/wishlist", handler_wishlist_add, pool);
    ROUTE_DELETE(router, "/api/wishlist/:product_id", handler_wishlist_remove, pool);
    ROUTE_GET(router, "/api/wishlist/check/:product_id", handler_wishlist_check, pool);
}

void handler_wishlist_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    /* Get wishlist items with product details */
    const char *query =
        "SELECT w.id, w.product_id, w.created_at, "
        "p.name AS product_name, p.slug AS product_slug, p.price, "
        "p.compare_at_price, p.stock_quantity, p.is_active, "
        "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS image_url "
        "FROM wishlist_items w "
        "JOIN products p ON w.product_id = p.id "
        "JOIN users u ON w.user_id = u.id "
        "WHERE u.id = $1 OR u.supabase_id = $1 "
        "ORDER BY w.created_at DESC";

    const char *params[] = { user_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *items = db_result_to_json(result);
    int item_count = PQntuples(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "items", items);
    cJSON_AddNumberToObject(data, "count", item_count);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_wishlist_add(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    int product_id = json_get_int(body, "product_id", 0);
    cJSON_Delete(body);

    if (product_id == 0) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "product_id required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check if product exists */
    char product_check[256];
    snprintf(product_check, sizeof(product_check),
             "SELECT id FROM products WHERE id = %d AND is_active = true", product_id);
    if (!db_exists(conn, product_check, 0, NULL)) {
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    /* Get user's internal ID */
    const char *user_query = "SELECT id FROM users WHERE id = $1 OR supabase_id = $1";
    const char *user_params[] = { user_id };
    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    const char *internal_user_id = db_result_value(user_result);
    char *user_id_copy = str_dup(internal_user_id);
    PQclear(user_result);

    /* Insert into wishlist (ignore if already exists) */
    const char *insert_query =
        "INSERT INTO wishlist_items (user_id, product_id) "
        "VALUES ($1, $2) "
        "ON CONFLICT (user_id, product_id) DO NOTHING "
        "RETURNING id";

    char product_str[16];
    snprintf(product_str, sizeof(product_str), "%d", product_id);

    const char *insert_params[] = { user_id_copy, product_str };
    PGresult *result = db_exec_params(conn, insert_query, 2, insert_params);

    free(user_id_copy);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to add to wishlist");
        return;
    }

    /* Check if item was inserted or already existed */
    bool was_inserted = db_result_has_rows(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "product_id", product_id);
    cJSON_AddBoolToObject(data, "added", was_inserted);
    cJSON_AddStringToObject(data, "message",
                           was_inserted ? "Added to wishlist" : "Already in wishlist");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, was_inserted ? HTTP_STATUS_CREATED : HTTP_STATUS_OK, json);
    free(json);
}

void handler_wishlist_remove(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *product_id_str = http_request_get_path_param(req, "product_id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (product_id_str == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Delete wishlist item (only if owned by user) */
    const char *delete_query =
        "DELETE FROM wishlist_items w "
        "USING users u "
        "WHERE w.product_id = $1 AND w.user_id = u.id "
        "AND (u.id = $2 OR u.supabase_id = $2) "
        "RETURNING w.id";
    const char *params[] = { product_id_str, user_id };

    PGresult *result = db_exec_params(conn, delete_query, 2, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Item not in wishlist");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
}

void handler_wishlist_check(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *product_id_str = http_request_get_path_param(req, "product_id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (product_id_str == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check if product is in wishlist */
    const char *check_query =
        "SELECT w.id FROM wishlist_items w "
        "JOIN users u ON w.user_id = u.id "
        "WHERE w.product_id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *params[] = { product_id_str, user_id };

    PGresult *result = db_exec_params(conn, check_query, 2, params);

    bool in_wishlist = db_result_ok(result) && db_result_has_rows(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "product_id", atoi(product_id_str));
    cJSON_AddBoolToObject(data, "in_wishlist", in_wishlist);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
