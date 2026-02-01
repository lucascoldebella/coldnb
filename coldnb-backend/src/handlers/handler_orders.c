#include "handlers/handler_orders.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Generate order number: ORD-YYYYMMDD-XXXXX */
static char *generate_order_number(void) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    char *random = str_random(5);
    if (random == NULL) {
        return NULL;
    }

    char *order_num = str_printf("ORD-%04d%02d%02d-%s",
                                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                                 random);
    free(random);
    return order_num;
}

void handler_orders_register(HttpRouter *router, DbPool *pool) {
    ROUTE_POST(router, "/api/orders", handler_orders_create, pool);
    ROUTE_GET(router, "/api/orders", handler_orders_list, pool);
    ROUTE_GET(router, "/api/orders/:id", handler_orders_get, pool);
}

void handler_orders_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    const char *shipping_address_id = json_get_string(body, "shipping_address_id", NULL);
    const char *discount_code = json_get_string(body, "discount_code", NULL);
    const char *payment_method = json_get_string(body, "payment_method", "card");
    const char *notes = json_get_string(body, "notes", NULL);

    if (str_is_empty(shipping_address_id)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "shipping_address_id required");
        return;
    }

    if (!uuid_validate(shipping_address_id)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid address ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get user's internal ID */
    const char *user_query = "SELECT id FROM users WHERE id = $1 OR supabase_id = $1";
    const char *user_params[] = { user_id };
    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    const char *internal_user_id = db_result_value(user_result);
    char *user_id_copy = str_dup(internal_user_id);
    PQclear(user_result);

    /* Get shipping address */
    const char *addr_query =
        "SELECT recipient_name, phone, street_address, street_address_2, "
        "city, state, postal_code, country "
        "FROM user_addresses WHERE id = $1 AND user_id = $2";
    const char *addr_params[] = { shipping_address_id, user_id_copy };
    PGresult *addr_result = db_exec_params(conn, addr_query, 2, addr_params);

    if (!db_result_ok(addr_result) || !db_result_has_rows(addr_result)) {
        PQclear(addr_result);
        free(user_id_copy);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Shipping address not found");
        return;
    }

    DbRow addr_row = { .result = addr_result, .row = 0 };
    const char *ship_name = db_row_get_string(&addr_row, "recipient_name");
    const char *ship_phone = db_row_get_string(&addr_row, "phone");
    const char *ship_street = db_row_get_string(&addr_row, "street_address");
    const char *ship_street2 = db_row_get_string(&addr_row, "street_address_2");
    const char *ship_city = db_row_get_string(&addr_row, "city");
    const char *ship_state = db_row_get_string(&addr_row, "state");
    const char *ship_postal = db_row_get_string(&addr_row, "postal_code");
    const char *ship_country = db_row_get_string(&addr_row, "country");

    /* Copy address data before clearing result */
    char *addr_name = str_dup(ship_name);
    char *addr_phone = ship_phone ? str_dup(ship_phone) : NULL;
    char *addr_street = str_dup(ship_street);
    char *addr_street2 = ship_street2 ? str_dup(ship_street2) : NULL;
    char *addr_city = str_dup(ship_city);
    char *addr_state = str_dup(ship_state);
    char *addr_postal = str_dup(ship_postal);
    char *addr_country = str_dup(ship_country);
    PQclear(addr_result);

    /* Get cart items */
    const char *cart_query =
        "SELECT ci.id, ci.product_id, ci.quantity, ci.color_id, ci.size_id, "
        "p.name, p.sku, p.price, p.stock_quantity, "
        "pc.name AS color_name, ps.name AS size_name, "
        "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS image_url "
        "FROM cart_items ci "
        "JOIN products p ON ci.product_id = p.id "
        "LEFT JOIN product_colors pc ON ci.color_id = pc.id "
        "LEFT JOIN product_sizes ps ON ci.size_id = ps.id "
        "WHERE ci.user_id = $1 AND p.is_active = true";
    const char *cart_params[] = { user_id_copy };
    PGresult *cart_result = db_exec_params(conn, cart_query, 1, cart_params);

    if (!db_result_ok(cart_result) || !db_result_has_rows(cart_result)) {
        PQclear(cart_result);
        free(user_id_copy);
        free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
        free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Cart is empty");
        return;
    }

    int item_count = PQntuples(cart_result);

    /* Calculate subtotal and validate stock */
    double subtotal = 0;
    for (int i = 0; i < item_count; i++) {
        DbRow row = { .result = cart_result, .row = i };
        int quantity = db_row_get_int(&row, "quantity");
        double price = db_row_get_double(&row, "price");
        int stock = db_row_get_int(&row, "stock_quantity");

        if (quantity > stock) {
            const char *product_name = db_row_get_string(&row, "name");
            PQclear(cart_result);
            free(user_id_copy);
            free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
            free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
            db_pool_release(pool, conn);
            cJSON_Delete(body);

            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "Insufficient stock for %s (available: %d)", product_name, stock);
            http_response_error(resp, HTTP_STATUS_BAD_REQUEST, error_msg);
            return;
        }

        subtotal += price * quantity;
    }

    /* Validate discount code if provided */
    double discount_amount = 0;
    char *validated_discount_code = NULL;

    if (discount_code != NULL && discount_code[0] != '\0') {
        const char *discount_query =
            "SELECT id, discount_type, discount_value, minimum_order, maximum_discount, "
            "usage_limit, used_count "
            "FROM discount_codes "
            "WHERE code = $1 AND is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (expires_at IS NULL OR expires_at > NOW())";
        const char *discount_params[] = { discount_code };
        PGresult *discount_result = db_exec_params(conn, discount_query, 1, discount_params);

        if (db_result_ok(discount_result) && db_result_has_rows(discount_result)) {
            DbRow d_row = { .result = discount_result, .row = 0 };
            const char *discount_type = db_row_get_string(&d_row, "discount_type");
            double discount_value = db_row_get_double(&d_row, "discount_value");
            double minimum_order = db_row_get_double(&d_row, "minimum_order");
            double maximum_discount = db_row_get_double(&d_row, "maximum_discount");
            int usage_limit = db_row_get_int(&d_row, "usage_limit");
            int used_count = db_row_get_int(&d_row, "used_count");

            bool valid = true;

            if (subtotal < minimum_order) {
                valid = false;
            }
            if (usage_limit > 0 && used_count >= usage_limit) {
                valid = false;
            }

            if (valid) {
                if (strcmp(discount_type, "percentage") == 0) {
                    discount_amount = subtotal * (discount_value / 100.0);
                } else {
                    discount_amount = discount_value;
                }

                if (maximum_discount > 0 && discount_amount > maximum_discount) {
                    discount_amount = maximum_discount;
                }

                validated_discount_code = str_dup(discount_code);
            }
        }
        PQclear(discount_result);
    }

    cJSON_Delete(body);

    /* Calculate totals */
    double shipping_cost = 0;  /* Could be calculated based on address/weight */
    double tax_amount = 0;     /* Could be calculated based on location */
    double total = subtotal - discount_amount + shipping_cost + tax_amount;

    /* Generate order number */
    char *order_number = generate_order_number();
    if (order_number == NULL) {
        PQclear(cart_result);
        free(user_id_copy);
        free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
        free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
        free(validated_discount_code);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to generate order number");
        return;
    }

    /* Begin transaction */
    if (!db_begin(conn)) {
        PQclear(cart_result);
        free(user_id_copy);
        free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
        free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
        free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* Insert order */
    const char *order_query =
        "INSERT INTO orders (order_number, user_id, payment_method, "
        "shipping_name, shipping_phone, shipping_street, shipping_street_2, "
        "shipping_city, shipping_state, shipping_postal_code, shipping_country, "
        "subtotal, shipping_cost, tax_amount, discount_amount, discount_code, "
        "total, notes) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18) "
        "RETURNING id";

    char subtotal_str[32], shipping_str[32], tax_str[32], discount_str[32], total_str[32];
    snprintf(subtotal_str, sizeof(subtotal_str), "%.2f", subtotal);
    snprintf(shipping_str, sizeof(shipping_str), "%.2f", shipping_cost);
    snprintf(tax_str, sizeof(tax_str), "%.2f", tax_amount);
    snprintf(discount_str, sizeof(discount_str), "%.2f", discount_amount);
    snprintf(total_str, sizeof(total_str), "%.2f", total);

    const char *order_params[] = {
        order_number, user_id_copy, payment_method,
        addr_name, addr_phone, addr_street, addr_street2,
        addr_city, addr_state, addr_postal, addr_country,
        subtotal_str, shipping_str, tax_str, discount_str, validated_discount_code,
        total_str, notes
    };

    PGresult *order_result = db_exec_params(conn, order_query, 18, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_rollback(conn);
        PQclear(cart_result);
        free(user_id_copy);
        free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
        free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
        free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create order");
        return;
    }

    const char *order_id_str = db_result_value(order_result);
    char *order_id = str_dup(order_id_str);
    PQclear(order_result);

    /* Insert order items and update stock */
    for (int i = 0; i < item_count; i++) {
        DbRow row = { .result = cart_result, .row = i };
        int product_id = db_row_get_int(&row, "product_id");
        int quantity = db_row_get_int(&row, "quantity");
        double price = db_row_get_double(&row, "price");
        const char *product_name = db_row_get_string(&row, "name");
        const char *product_sku = db_row_get_string(&row, "sku");
        const char *image_url = db_row_get_string(&row, "image_url");
        const char *color_name = db_row_get_string(&row, "color_name");
        const char *size_name = db_row_get_string(&row, "size_name");

        double line_total = price * quantity;

        /* Insert order item */
        const char *item_query =
            "INSERT INTO order_items (order_id, product_id, product_name, product_sku, "
            "product_image, color_name, size_name, quantity, unit_price, total_price) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)";

        char pid_str[16], qty_str[16], price_str[32], line_total_str[32];
        snprintf(pid_str, sizeof(pid_str), "%d", product_id);
        snprintf(qty_str, sizeof(qty_str), "%d", quantity);
        snprintf(price_str, sizeof(price_str), "%.2f", price);
        snprintf(line_total_str, sizeof(line_total_str), "%.2f", line_total);

        const char *item_params[] = {
            order_id, pid_str, product_name, product_sku, image_url,
            color_name, size_name, qty_str, price_str, line_total_str
        };

        PGresult *item_result = db_exec_params(conn, item_query, 10, item_params);
        if (!db_result_ok(item_result)) {
            PQclear(item_result);
            db_rollback(conn);
            PQclear(cart_result);
            free(order_id);
            free(user_id_copy);
            free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
            free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
            free(validated_discount_code);
            free(order_number);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create order items");
            return;
        }
        PQclear(item_result);

        /* Update product stock */
        const char *stock_query =
            "UPDATE products SET stock_quantity = stock_quantity - $1 WHERE id = $2";
        const char *stock_params[] = { qty_str, pid_str };
        PGresult *stock_result = db_exec_params(conn, stock_query, 2, stock_params);
        PQclear(stock_result);
    }

    PQclear(cart_result);

    /* Add order history entry */
    const char *history_query =
        "INSERT INTO order_history (order_id, status, notes) "
        "VALUES ($1, 'pending', 'Order created')";
    const char *history_params[] = { order_id };
    PGresult *history_result = db_exec_params(conn, history_query, 1, history_params);
    PQclear(history_result);

    /* Clear user's cart */
    const char *clear_query = "DELETE FROM cart_items WHERE user_id = $1";
    const char *clear_params[] = { user_id_copy };
    PGresult *clear_result = db_exec_params(conn, clear_query, 1, clear_params);
    PQclear(clear_result);

    /* Update discount code usage if used */
    if (validated_discount_code != NULL) {
        const char *discount_update =
            "UPDATE discount_codes SET used_count = used_count + 1 WHERE code = $1";
        const char *discount_update_params[] = { validated_discount_code };
        PGresult *discount_update_result = db_exec_params(conn, discount_update, 1, discount_update_params);
        PQclear(discount_update_result);
    }

    /* Commit transaction */
    if (!db_commit(conn)) {
        db_rollback(conn);
        free(order_id);
        free(user_id_copy);
        free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
        free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
        free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to commit order");
        return;
    }

    db_pool_release(pool, conn);

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", order_id);
    cJSON_AddStringToObject(data, "order_number", order_number);
    cJSON_AddStringToObject(data, "status", "pending");
    cJSON_AddStringToObject(data, "payment_status", "pending");
    cJSON_AddNumberToObject(data, "subtotal", subtotal);
    cJSON_AddNumberToObject(data, "shipping_cost", shipping_cost);
    cJSON_AddNumberToObject(data, "tax_amount", tax_amount);
    cJSON_AddNumberToObject(data, "discount_amount", discount_amount);
    cJSON_AddNumberToObject(data, "total", total);
    cJSON_AddNumberToObject(data, "item_count", item_count);

    free(order_id);
    free(user_id_copy);
    free(addr_name); free(addr_phone); free(addr_street); free(addr_street2);
    free(addr_city); free(addr_state); free(addr_postal); free(addr_country);
    free(validated_discount_code);
    free(order_number);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Order created: %s", order_number);
}

void handler_orders_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    /* Pagination parameters */
    const char *page_str = http_request_get_query_param(req, "page");
    const char *limit_str = http_request_get_query_param(req, "limit");

    int page = page_str ? atoi(page_str) : 1;
    int limit = limit_str ? atoi(limit_str) : 10;

    if (page < 1) page = 1;
    if (limit < 1) limit = 10;
    if (limit > 50) limit = 50;

    int offset = (page - 1) * limit;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Count total orders */
    const char *count_query =
        "SELECT COUNT(*) FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE u.id = $1 OR u.supabase_id = $1";
    const char *count_params[] = { user_id };
    int total = db_count(conn, count_query, 1, count_params);

    /* Get orders */
    char limit_str_buf[16], offset_str_buf[16];
    snprintf(limit_str_buf, sizeof(limit_str_buf), "%d", limit);
    snprintf(offset_str_buf, sizeof(offset_str_buf), "%d", offset);

    const char *query =
        "SELECT o.id, o.order_number, o.status, o.payment_status, o.payment_method, "
        "o.subtotal, o.shipping_cost, o.discount_amount, o.total, "
        "o.created_at, "
        "(SELECT COUNT(*) FROM order_items WHERE order_id = o.id) AS item_count "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE u.id = $1 OR u.supabase_id = $1 "
        "ORDER BY o.created_at DESC "
        "LIMIT $2 OFFSET $3";

    const char *params[] = { user_id, limit_str_buf, offset_str_buf };
    PGresult *result = db_exec_params(conn, query, 3, params);

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

void handler_orders_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *order_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
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

    /* Get order (only if owned by user) */
    const char *order_query =
        "SELECT o.id, o.order_number, o.status, o.payment_status, o.payment_method, "
        "o.shipping_name, o.shipping_phone, o.shipping_street, o.shipping_street_2, "
        "o.shipping_city, o.shipping_state, o.shipping_postal_code, o.shipping_country, "
        "o.subtotal, o.shipping_cost, o.tax_amount, o.discount_amount, o.discount_code, "
        "o.total, o.notes, o.created_at, o.paid_at, o.shipped_at, o.delivered_at "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *order_params[] = { order_id, user_id };

    PGresult *order_result = db_exec_params(conn, order_query, 2, order_params);

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
        "SELECT id, product_id, product_name, product_sku, product_image, "
        "color_name, size_name, quantity, unit_price, total_price "
        "FROM order_items WHERE order_id = $1 "
        "ORDER BY id";
    const char *items_params[] = { order_id };

    PGresult *items_result = db_exec_params(conn, items_query, 1, items_params);

    if (db_result_ok(items_result)) {
        cJSON *items = db_result_to_json(items_result);
        cJSON_AddItemToObject(data, "items", items);
    }
    PQclear(items_result);

    /* Get order history */
    const char *history_query =
        "SELECT status, notes, created_at "
        "FROM order_history WHERE order_id = $1 "
        "ORDER BY created_at DESC";
    const char *history_params[] = { order_id };

    PGresult *history_result = db_exec_params(conn, history_query, 1, history_params);

    if (db_result_ok(history_result)) {
        cJSON *history = db_result_to_json(history_result);
        cJSON_AddItemToObject(data, "history", history);
    }
    PQclear(history_result);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
