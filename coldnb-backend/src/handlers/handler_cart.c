#include "handlers/handler_cart.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_cart_register(HttpRouter *router, DbPool *pool) {
    /* All cart routes require authentication - middleware handles this */
    ROUTE_GET(router, "/api/cart", handler_cart_get, pool);
    ROUTE_GET(router, "/api/cart/count", handler_cart_count, pool);
    ROUTE_POST(router, "/api/cart", handler_cart_add, pool);
    ROUTE_PUT(router, "/api/cart/:id", handler_cart_update, pool);
    ROUTE_DELETE(router, "/api/cart/:id", handler_cart_remove, pool);
    ROUTE_DELETE(router, "/api/cart", handler_cart_clear, pool);
}

void handler_cart_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    /* Get cart items with product details */
    const char *query =
        "SELECT ci.id, ci.product_id, ci.quantity, ci.color_id, ci.size_id, "
        "p.name AS product_name, p.slug AS product_slug, p.price, p.compare_at_price, "
        "p.stock_quantity AS available_stock, "
        "pc.name AS color_name, pc.hex_code, "
        "ps.name AS size_name, "
        "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS image_url "
        "FROM cart_items ci "
        "JOIN products p ON ci.product_id = p.id "
        "LEFT JOIN product_colors pc ON ci.color_id = pc.id "
        "LEFT JOIN product_sizes ps ON ci.size_id = ps.id "
        "WHERE ci.user_id = $1 "
        "ORDER BY ci.created_at DESC";

    const char *params[] = { user_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    /* Build cart response */
    cJSON *items = db_result_to_json(result);
    int item_count = PQntuples(result);
    PQclear(result);

    /* Calculate totals */
    double subtotal = 0;
    int total_quantity = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, items) {
        double price = json_get_double(item, "price", 0);
        int qty = json_get_int(item, "quantity", 1);
        subtotal += price * qty;
        total_quantity += qty;

        /* Add line_total to each item */
        cJSON_AddNumberToObject(item, "line_total", price * qty);
    }

    db_pool_release(pool, conn);

    /* Build response */
    cJSON *cart = cJSON_CreateObject();
    cJSON_AddItemToObject(cart, "items", items);
    cJSON_AddNumberToObject(cart, "item_count", item_count);
    cJSON_AddNumberToObject(cart, "total_quantity", total_quantity);
    cJSON_AddNumberToObject(cart, "subtotal", subtotal);

    cJSON *response = json_create_success(cart);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_cart_add(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    int product_id = json_get_int(body, "product_id", 0);
    int quantity = json_get_int(body, "quantity", 1);
    int color_id = json_get_int(body, "color_id", 0);
    int size_id = json_get_int(body, "size_id", 0);

    cJSON_Delete(body);

    if (product_id == 0) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "product_id required");
        return;
    }

    if (quantity < 1) quantity = 1;
    if (quantity > 99) quantity = 99;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check if product exists and is active */
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT id FROM products WHERE id = %d AND is_active = true", product_id);
    if (!db_exists(conn, check_query, 0, NULL)) {
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    /* Upsert cart item */
    const char *upsert_query =
        "INSERT INTO cart_items (user_id, product_id, quantity, color_id, size_id) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (user_id, product_id, color_id, size_id) "
        "DO UPDATE SET quantity = cart_items.quantity + EXCLUDED.quantity, updated_at = NOW() "
        "RETURNING id";

    char product_str[16], qty_str[16], color_str[16], size_str[16];
    snprintf(product_str, sizeof(product_str), "%d", product_id);
    snprintf(qty_str, sizeof(qty_str), "%d", quantity);

    const char *color_param = NULL;
    const char *size_param = NULL;

    if (color_id > 0) {
        snprintf(color_str, sizeof(color_str), "%d", color_id);
        color_param = color_str;
    }
    if (size_id > 0) {
        snprintf(size_str, sizeof(size_str), "%d", size_id);
        size_param = size_str;
    }

    const char *params[] = { user_id, product_str, qty_str, color_param, size_param };
    PGresult *result = db_exec_params(conn, upsert_query, 5, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to add to cart");
        return;
    }

    const char *item_id = db_result_value(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", item_id);
    cJSON_AddStringToObject(data, "message", "Item added to cart");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

void handler_cart_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *item_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (item_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Item ID required");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    int quantity = json_get_int(body, "quantity", 0);
    cJSON_Delete(body);

    if (quantity < 1 || quantity > 99) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid quantity");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Update cart item (only if owned by user) */
    const char *update_query =
        "UPDATE cart_items SET quantity = $1, updated_at = NOW() "
        "WHERE id = $2 AND user_id = $3 RETURNING id";

    char qty_str[16];
    snprintf(qty_str, sizeof(qty_str), "%d", quantity);

    const char *params[] = { qty_str, item_id, user_id };
    PGresult *result = db_exec_params(conn, update_query, 3, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Cart item not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_success(resp, "{\"message\":\"Cart updated\"}");
}

void handler_cart_remove(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *item_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (item_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Item ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Delete cart item (only if owned by user) */
    const char *delete_query =
        "DELETE FROM cart_items WHERE id = $1 AND user_id = $2 RETURNING id";

    const char *params[] = { item_id, user_id };
    PGresult *result = db_exec_params(conn, delete_query, 2, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Cart item not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
}

void handler_cart_clear(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    /* Delete all cart items for user */
    const char *delete_query = "DELETE FROM cart_items WHERE user_id = $1";

    const char *params[] = { user_id };
    PGresult *result = db_exec_params(conn, delete_query, 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to clear cart");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
}

void handler_cart_count(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    const char *query = "SELECT COALESCE(SUM(quantity), 0) FROM cart_items WHERE user_id = $1";
    const char *params[] = { user_id };

    int count = db_count(conn, query, 1, params);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "count", count);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
