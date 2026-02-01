#include "handlers/handler_admin_products.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_admin_products_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/products", handler_admin_products_list, pool);
    ROUTE_GET(router, "/api/admin/products/:id", handler_admin_products_get, pool);
    ROUTE_POST(router, "/api/admin/products", handler_admin_products_create, pool);
    ROUTE_PUT(router, "/api/admin/products/:id", handler_admin_products_update, pool);
    ROUTE_DELETE(router, "/api/admin/products/:id", handler_admin_products_delete, pool);
    ROUTE_POST(router, "/api/admin/products/:id/images", handler_admin_products_add_image, pool);
    ROUTE_DELETE(router, "/api/admin/products/:id/images/:image_id", handler_admin_products_remove_image, pool);
}

void handler_admin_products_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *page_str = http_request_get_query_param(req, "page");
    const char *limit_str = http_request_get_query_param(req, "limit");
    const char *search = http_request_get_query_param(req, "search");
    const char *category_id = http_request_get_query_param(req, "category_id");
    const char *show_inactive = http_request_get_query_param(req, "show_inactive");

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

    /* Build WHERE clause */
    char where_clause[512];
    size_t where_len = 0;
    int param_index = 0;
    const char *params[4];

    where_len = (size_t)snprintf(where_clause, sizeof(where_clause), "WHERE 1=1");

    if (show_inactive == NULL || strcmp(show_inactive, "true") != 0) {
        where_len += (size_t)snprintf(where_clause + where_len, sizeof(where_clause) - where_len,
                                       " AND p.is_active = true");
    }

    if (category_id != NULL) {
        param_index++;
        where_len += (size_t)snprintf(where_clause + where_len, sizeof(where_clause) - where_len,
                 " AND p.category_id = $%d", param_index);
        params[param_index - 1] = category_id;
    }

    char *search_pattern = NULL;
    if (search != NULL && search[0] != '\0') {
        param_index++;
        where_len += (size_t)snprintf(where_clause + where_len, sizeof(where_clause) - where_len,
                 " AND (p.name ILIKE $%d OR p.sku ILIKE $%d)", param_index, param_index);
        search_pattern = str_printf("%%%s%%", search);
        params[param_index - 1] = search_pattern;
    }
    (void)where_len; /* Suppress unused warning after final use */

    /* Count query */
    char count_query[1024];
    snprintf(count_query, sizeof(count_query),
             "SELECT COUNT(*) FROM products p %s", where_clause);

    int total = db_count(conn, count_query, param_index, params);

    /* Main query */
    char query[2048];
    snprintf(query, sizeof(query),
             "SELECT p.id, p.name, p.slug, p.sku, p.price, p.compare_at_price, "
             "p.stock_quantity, p.is_active, p.is_featured, p.created_at, "
             "c.name AS category_name, "
             "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS image_url "
             "FROM products p "
             "LEFT JOIN categories c ON p.category_id = c.id "
             "%s "
             "ORDER BY p.created_at DESC "
             "LIMIT %s OFFSET %s",
             where_clause, limit_str_buf, offset_str_buf);

    PGresult *result = db_exec_params(conn, query, param_index, params);
    free(search_pattern);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *products = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    int total_pages = (total + limit - 1) / limit;

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "products", products);
    cJSON_AddItemToObject(data, "pagination",
                         json_create_pagination(page, limit, total, total_pages));

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_products_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *product_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (product_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT p.*, c.name AS category_name "
        "FROM products p "
        "LEFT JOIN categories c ON p.category_id = c.id "
        "WHERE p.id = $1";
    const char *params[] = { product_id };

    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);

    /* Get images */
    const char *images_query =
        "SELECT * FROM product_images WHERE product_id = $1 ORDER BY sort_order, id";
    PGresult *images_result = db_exec_params(conn, images_query, 1, params);
    if (db_result_ok(images_result)) {
        cJSON_AddItemToObject(data, "images", db_result_to_json(images_result));
    }
    PQclear(images_result);

    /* Get colors */
    const char *colors_query =
        "SELECT * FROM product_colors WHERE product_id = $1 ORDER BY sort_order, id";
    PGresult *colors_result = db_exec_params(conn, colors_query, 1, params);
    if (db_result_ok(colors_result)) {
        cJSON_AddItemToObject(data, "colors", db_result_to_json(colors_result));
    }
    PQclear(colors_result);

    /* Get sizes */
    const char *sizes_query =
        "SELECT * FROM product_sizes WHERE product_id = $1 ORDER BY sort_order, id";
    PGresult *sizes_result = db_exec_params(conn, sizes_query, 1, params);
    if (db_result_ok(sizes_result)) {
        cJSON_AddItemToObject(data, "sizes", db_result_to_json(sizes_result));
    }
    PQclear(sizes_result);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_products_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *name = json_get_string(body, "name", NULL);
    const char *slug = json_get_string(body, "slug", NULL);

    if (str_is_empty(name)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name required");
        return;
    }

    /* Generate slug if not provided */
    char *generated_slug = NULL;
    if (str_is_empty(slug)) {
        generated_slug = str_dup(name);
        str_to_lower(generated_slug);
        /* Replace spaces with hyphens */
        for (char *p = generated_slug; *p; p++) {
            if (*p == ' ') *p = '-';
        }
        slug = generated_slug;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        free(generated_slug);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *insert_query =
        "INSERT INTO products (name, slug, description, short_description, sku, "
        "price, compare_at_price, cost_price, category_id, brand, stock_quantity, "
        "is_active, is_featured, is_new, is_sale, meta_title, meta_description) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17) "
        "RETURNING id, name, slug";

    char price_str[32], compare_str[32], cost_str[32], stock_str[16];
    char category_str[16];

    double price = json_get_double(body, "price", 0);
    snprintf(price_str, sizeof(price_str), "%.2f", price);

    double compare = json_get_double(body, "compare_at_price", 0);
    const char *compare_param = compare > 0 ? (snprintf(compare_str, sizeof(compare_str), "%.2f", compare), compare_str) : NULL;

    double cost = json_get_double(body, "cost_price", 0);
    const char *cost_param = cost > 0 ? (snprintf(cost_str, sizeof(cost_str), "%.2f", cost), cost_str) : NULL;

    int category_id = json_get_int(body, "category_id", 0);
    const char *category_param = category_id > 0 ? (snprintf(category_str, sizeof(category_str), "%d", category_id), category_str) : NULL;

    int stock = json_get_int(body, "stock_quantity", 0);
    snprintf(stock_str, sizeof(stock_str), "%d", stock);

    const char *params[] = {
        name,
        slug,
        json_get_string(body, "description", NULL),
        json_get_string(body, "short_description", NULL),
        json_get_string(body, "sku", NULL),
        price_str,
        compare_param,
        cost_param,
        category_param,
        json_get_string(body, "brand", NULL),
        stock_str,
        json_get_bool(body, "is_active", true) ? "true" : "false",
        json_get_bool(body, "is_featured", false) ? "true" : "false",
        json_get_bool(body, "is_new", false) ? "true" : "false",
        json_get_bool(body, "is_sale", false) ? "true" : "false",
        json_get_string(body, "meta_title", NULL),
        json_get_string(body, "meta_description", NULL)
    };

    PGresult *result = db_exec_params(conn, insert_query, 17, params);

    cJSON_Delete(body);
    free(generated_slug);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        const char *error = PQerrorMessage(conn);
        PQclear(result);
        db_pool_release(pool, conn);

        if (strstr(error, "unique") != NULL && strstr(error, "slug") != NULL) {
            http_response_error(resp, HTTP_STATUS_CONFLICT, "Product slug already exists");
        } else {
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create product");
        }
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Product created: %s", name);
}

void handler_admin_products_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *product_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (product_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
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

    /* Build dynamic update - only update provided fields */
    const char *update_query =
        "UPDATE products SET "
        "name = COALESCE($2, name), "
        "slug = COALESCE($3, slug), "
        "description = COALESCE($4, description), "
        "short_description = COALESCE($5, short_description), "
        "sku = COALESCE($6, sku), "
        "price = COALESCE($7, price), "
        "compare_at_price = COALESCE($8, compare_at_price), "
        "cost_price = COALESCE($9, cost_price), "
        "category_id = COALESCE($10, category_id), "
        "brand = COALESCE($11, brand), "
        "stock_quantity = COALESCE($12, stock_quantity), "
        "is_active = COALESCE($13, is_active), "
        "is_featured = COALESCE($14, is_featured), "
        "is_new = COALESCE($15, is_new), "
        "is_sale = COALESCE($16, is_sale), "
        "updated_at = NOW() "
        "WHERE id = $1 "
        "RETURNING id, name, slug";

    char price_str[32], compare_str[32], cost_str[32], stock_str[16], category_str[16];
    const char *price_param = NULL, *compare_param = NULL, *cost_param = NULL;
    const char *stock_param = NULL, *category_param = NULL;
    const char *is_active_str = NULL, *is_featured_str = NULL;
    const char *is_new_str = NULL, *is_sale_str = NULL;

    if (cJSON_HasObjectItem(body, "price")) {
        snprintf(price_str, sizeof(price_str), "%.2f", json_get_double(body, "price", 0));
        price_param = price_str;
    }
    if (cJSON_HasObjectItem(body, "compare_at_price")) {
        snprintf(compare_str, sizeof(compare_str), "%.2f", json_get_double(body, "compare_at_price", 0));
        compare_param = compare_str;
    }
    if (cJSON_HasObjectItem(body, "cost_price")) {
        snprintf(cost_str, sizeof(cost_str), "%.2f", json_get_double(body, "cost_price", 0));
        cost_param = cost_str;
    }
    if (cJSON_HasObjectItem(body, "stock_quantity")) {
        snprintf(stock_str, sizeof(stock_str), "%d", json_get_int(body, "stock_quantity", 0));
        stock_param = stock_str;
    }
    if (cJSON_HasObjectItem(body, "category_id")) {
        snprintf(category_str, sizeof(category_str), "%d", json_get_int(body, "category_id", 0));
        category_param = category_str;
    }
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", true) ? "true" : "false";
    }
    if (cJSON_HasObjectItem(body, "is_featured")) {
        is_featured_str = json_get_bool(body, "is_featured", false) ? "true" : "false";
    }
    if (cJSON_HasObjectItem(body, "is_new")) {
        is_new_str = json_get_bool(body, "is_new", false) ? "true" : "false";
    }
    if (cJSON_HasObjectItem(body, "is_sale")) {
        is_sale_str = json_get_bool(body, "is_sale", false) ? "true" : "false";
    }

    const char *params[] = {
        product_id,
        json_get_string(body, "name", NULL),
        json_get_string(body, "slug", NULL),
        json_get_string(body, "description", NULL),
        json_get_string(body, "short_description", NULL),
        json_get_string(body, "sku", NULL),
        price_param,
        compare_param,
        cost_param,
        category_param,
        json_get_string(body, "brand", NULL),
        stock_param,
        is_active_str,
        is_featured_str,
        is_new_str,
        is_sale_str
    };

    PGresult *result = db_exec_params(conn, update_query, 16, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
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

    LOG_INFO("Product updated: %s", product_id);
}

void handler_admin_products_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *product_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (product_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Soft delete - set is_active to false */
    const char *query =
        "UPDATE products SET is_active = false, updated_at = NOW() "
        "WHERE id = $1 RETURNING id";
    const char *params[] = { product_id };

    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);

    LOG_INFO("Product deleted (soft): %s", product_id);
}

void handler_admin_products_add_image(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *product_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (product_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *url = json_get_string(body, "url", NULL);
    if (str_is_empty(url)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "URL required");
        return;
    }

    const char *alt_text = json_get_string(body, "alt_text", NULL);
    bool is_primary = json_get_bool(body, "is_primary", false);
    int sort_order = json_get_int(body, "sort_order", 0);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check product exists */
    char check_query[128];
    snprintf(check_query, sizeof(check_query), "SELECT id FROM products WHERE id = %s", product_id);
    if (!db_exists(conn, check_query, 0, NULL)) {
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    if (!db_begin(conn)) {
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* If setting as primary, unset existing primary */
    if (is_primary) {
        const char *unset_query =
            "UPDATE product_images SET is_primary = false WHERE product_id = $1";
        const char *unset_params[] = { product_id };
        PGresult *unset_result = db_exec_params(conn, unset_query, 1, unset_params);
        PQclear(unset_result);
    }

    const char *insert_query =
        "INSERT INTO product_images (product_id, url, alt_text, sort_order, is_primary) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, url, alt_text, sort_order, is_primary";

    char sort_str[16];
    snprintf(sort_str, sizeof(sort_str), "%d", sort_order);

    const char *params[] = { product_id, url, alt_text, sort_str, is_primary ? "true" : "false" };
    PGresult *result = db_exec_params(conn, insert_query, 5, params);

    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_rollback(conn);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to add image");
        return;
    }

    db_commit(conn);

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

void handler_admin_products_remove_image(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *product_id = http_request_get_path_param(req, "id");
    const char *image_id = http_request_get_path_param(req, "image_id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (product_id == NULL || image_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID and Image ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *delete_query =
        "DELETE FROM product_images WHERE id = $1 AND product_id = $2 RETURNING id";
    const char *params[] = { image_id, product_id };

    PGresult *result = db_exec_params(conn, delete_query, 2, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Image not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
}
