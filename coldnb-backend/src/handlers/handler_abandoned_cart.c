#include "handlers/handler_abandoned_cart.h"
#include "services/svc_email.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_abandoned_cart_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/abandoned-carts", handler_admin_abandoned_carts_list, pool);
    ROUTE_POST(router, "/api/admin/abandoned-carts/send", handler_admin_abandoned_carts_send, pool);
}

void handler_admin_abandoned_carts_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    (void)req;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Find users with cart items idle > 24h who haven't ordered in the last 24h
       and haven't received an abandoned cart email in the last 3 days */
    const char *query =
        "SELECT u.id AS user_id, u.email, u.full_name, "
        "COUNT(ci.id) AS item_count, "
        "SUM(p.price * ci.quantity) AS cart_total, "
        "MAX(ci.updated_at) AS last_activity, "
        "(SELECT name FROM products p2 "
        " JOIN cart_items ci2 ON ci2.product_id = p2.id "
        " WHERE ci2.user_id = u.id ORDER BY p2.price DESC LIMIT 1) AS top_product, "
        "(SELECT pi.url FROM product_images pi "
        " JOIN cart_items ci3 ON ci3.product_id = pi.product_id "
        " WHERE ci3.user_id = u.id AND pi.is_primary = true "
        " ORDER BY (SELECT price FROM products WHERE id = ci3.product_id) DESC LIMIT 1) AS top_image "
        "FROM cart_items ci "
        "JOIN users u ON ci.user_id = u.id "
        "JOIN products p ON ci.product_id = p.id "
        "WHERE ci.updated_at < NOW() - INTERVAL '24 hours' "
        "AND u.is_active = true "
        "AND NOT EXISTS ("
        "  SELECT 1 FROM orders o WHERE o.user_id = u.id "
        "  AND o.created_at > ci.updated_at"
        ") "
        "AND NOT EXISTS ("
        "  SELECT 1 FROM abandoned_cart_emails ace "
        "  WHERE ace.user_id = u.id AND ace.sent_at > NOW() - INTERVAL '3 days'"
        ") "
        "GROUP BY u.id, u.email, u.full_name "
        "ORDER BY cart_total DESC "
        "LIMIT 100";

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
        cJSON_AddStringToObject(item, "user_id", db_row_get_string(&row, "user_id"));
        cJSON_AddStringToObject(item, "email", db_row_get_string(&row, "email"));

        const char *name = db_row_get_string(&row, "full_name");
        if (!str_is_empty(name)) {
            cJSON_AddStringToObject(item, "full_name", name);
        } else {
            cJSON_AddNullToObject(item, "full_name");
        }

        cJSON_AddNumberToObject(item, "item_count", db_row_get_int(&row, "item_count"));
        cJSON_AddNumberToObject(item, "cart_total", db_row_get_double(&row, "cart_total"));
        cJSON_AddStringToObject(item, "last_activity", db_row_get_string(&row, "last_activity"));

        const char *top = db_row_get_string(&row, "top_product");
        if (!str_is_empty(top)) {
            cJSON_AddStringToObject(item, "top_product", top);
        } else {
            cJSON_AddNullToObject(item, "top_product");
        }

        const char *img = db_row_get_string(&row, "top_image");
        if (!str_is_empty(img)) {
            cJSON_AddStringToObject(item, "top_image", img);
        } else {
            cJSON_AddNullToObject(item, "top_image");
        }

        cJSON_AddItemToArray(arr, item);
    }

    PQclear(result);

    cJSON *response = json_create_success(arr);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_abandoned_carts_send(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    (void)req;

    if (!email_service_is_initialized()) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Email service not configured");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Same query as list but we only need email data */
    const char *query =
        "SELECT u.id AS user_id, u.email, u.full_name, "
        "COUNT(ci.id) AS item_count, "
        "SUM(p.price * ci.quantity) AS cart_total, "
        "(SELECT name FROM products p2 "
        " JOIN cart_items ci2 ON ci2.product_id = p2.id "
        " WHERE ci2.user_id = u.id ORDER BY p2.price DESC LIMIT 1) AS top_product, "
        "(SELECT pi.url FROM product_images pi "
        " JOIN cart_items ci3 ON ci3.product_id = pi.product_id "
        " WHERE ci3.user_id = u.id AND pi.is_primary = true "
        " ORDER BY (SELECT price FROM products WHERE id = ci3.product_id) DESC LIMIT 1) AS top_image "
        "FROM cart_items ci "
        "JOIN users u ON ci.user_id = u.id "
        "JOIN products p ON ci.product_id = p.id "
        "WHERE ci.updated_at < NOW() - INTERVAL '24 hours' "
        "AND u.is_active = true "
        "AND NOT EXISTS ("
        "  SELECT 1 FROM orders o WHERE o.user_id = u.id "
        "  AND o.created_at > ci.updated_at"
        ") "
        "AND NOT EXISTS ("
        "  SELECT 1 FROM abandoned_cart_emails ace "
        "  WHERE ace.user_id = u.id AND ace.sent_at > NOW() - INTERVAL '3 days'"
        ") "
        "GROUP BY u.id, u.email, u.full_name "
        "ORDER BY cart_total DESC "
        "LIMIT 50";

    PGresult *result = db_exec_params(conn, query, 0, NULL);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    int sent = 0;
    int failed = 0;

    for (int i = 0; i < rows; i++) {
        DbRow row = { .result = result, .row = i };

        char *user_id = str_dup(db_row_get_string(&row, "user_id"));
        char *email = str_dup(db_row_get_string(&row, "email"));
        char *name = str_dup(db_row_get_string(&row, "full_name"));
        int item_count = db_row_get_int(&row, "item_count");
        double cart_total = db_row_get_double(&row, "cart_total");
        char *top_product = str_dup(db_row_get_string(&row, "top_product"));
        char *top_image = str_dup(db_row_get_string(&row, "top_image"));

        EmailAbandonedCart cart_email = {
            .customer_email = email,
            .customer_name = name,
            .item_count = item_count,
            .cart_total = cart_total,
            .top_product_name = top_product,
            .top_product_image = top_image,
        };

        int send_result = email_service_send_abandoned_cart(&cart_email);

        if (send_result == 0) {
            /* Record that we sent the email */
            char item_str[16];
            snprintf(item_str, sizeof(item_str), "%d", item_count);
            char total_str[32];
            snprintf(total_str, sizeof(total_str), "%.2f", cart_total);

            const char *insert_query =
                "INSERT INTO abandoned_cart_emails (user_id, email, cart_item_count, cart_total) "
                "VALUES ($1, $2, $3::int, $4::numeric)";
            const char *insert_params[] = { user_id, email, item_str, total_str };
            PGresult *insert_result = db_exec_params(conn, insert_query, 4, insert_params);
            PQclear(insert_result);
            sent++;
        } else {
            failed++;
        }

        free(user_id);
        free(email);
        free(name);
        free(top_product);
        free(top_image);
    }

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "eligible", rows);
    cJSON_AddNumberToObject(data, "sent", sent);
    cJSON_AddNumberToObject(data, "failed", failed);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Abandoned cart emails: %d sent, %d failed out of %d eligible", sent, failed, rows);
}
