#include "handlers/handler_products.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

/* Build product JSON with related data */
static cJSON *product_to_json(PGconn *conn, DbRow *row, bool include_details) {
    cJSON *product = db_row_to_json(row);
    if (product == NULL) {
        return NULL;
    }

    int product_id = db_row_get_int(row, "id");

    /* Get images */
    const char *img_query =
        "SELECT id, url, alt_text, is_primary FROM product_images "
        "WHERE product_id = $1 ORDER BY sort_order, id";
    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%d", product_id);
    const char *params[] = { id_str };

    PGresult *img_result = db_exec_params(conn, img_query, 1, params);
    if (db_result_ok(img_result)) {
        cJSON *images = db_result_to_json(img_result);
        cJSON_AddItemToObject(product, "images", images);
    }
    PQclear(img_result);

    if (include_details) {
        /* Get colors */
        const char *color_query =
            "SELECT id, name, hex_code, image_url, stock_quantity FROM product_colors "
            "WHERE product_id = $1 ORDER BY sort_order, id";

        PGresult *color_result = db_exec_params(conn, color_query, 1, params);
        if (db_result_ok(color_result)) {
            cJSON *colors = db_result_to_json(color_result);
            cJSON_AddItemToObject(product, "colors", colors);
        }
        PQclear(color_result);

        /* Get sizes */
        const char *size_query =
            "SELECT id, name, stock_quantity FROM product_sizes "
            "WHERE product_id = $1 ORDER BY sort_order, id";

        PGresult *size_result = db_exec_params(conn, size_query, 1, params);
        if (db_result_ok(size_result)) {
            cJSON *sizes = db_result_to_json(size_result);
            cJSON_AddItemToObject(product, "sizes", sizes);
        }
        PQclear(size_result);
    }

    return product;
}

void handler_products_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/products", handler_products_list, pool);
    ROUTE_GET(router, "/api/products/search", handler_products_search, pool);
    ROUTE_GET(router, "/api/products/featured", handler_products_featured, pool);
    ROUTE_GET(router, "/api/products/:id", handler_products_get, pool);
    ROUTE_GET(router, "/api/categories", handler_categories_list, pool);
    ROUTE_GET(router, "/api/categories/:slug/products", handler_categories_products, pool);
}

void handler_products_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Parse query parameters */
    const char *page_str = http_request_get_query_param(req, "page");
    const char *per_page_str = http_request_get_query_param(req, "per_page");
    const char *category_str = http_request_get_query_param(req, "category");
    const char *sort = http_request_get_query_param(req, "sort");
    const char *min_price = http_request_get_query_param(req, "min_price");
    const char *max_price = http_request_get_query_param(req, "max_price");
    const char *is_featured = http_request_get_query_param(req, "featured");
    const char *is_sale = http_request_get_query_param(req, "sale");

    int page = page_str ? atoi(page_str) : 1;
    int per_page = per_page_str ? atoi(per_page_str) : 20;
    if (per_page > 100) per_page = 100;
    if (per_page < 1) per_page = 20;
    if (page < 1) page = 1;

    /* Build WHERE clause safely using snprintf */
    char where[1024];
    size_t where_len = 0;
    where_len = (size_t)snprintf(where, sizeof(where), "is_active = true");

    if (category_str != NULL && where_len < sizeof(where) - 1) {
        char *escaped = db_escape_string(conn, category_str);
        if (escaped) {
            where_len += (size_t)snprintf(where + where_len, sizeof(where) - where_len,
                     " AND category_id = (SELECT id FROM categories WHERE slug = '%s')", escaped);
            free(escaped);
        }
    }

    if (min_price != NULL && where_len < sizeof(where) - 1) {
        /* Validate min_price is numeric to prevent injection */
        bool valid = true;
        for (const char *p = min_price; *p && valid; p++) {
            if ((*p < '0' || *p > '9') && *p != '.') valid = false;
        }
        if (valid) {
            where_len += (size_t)snprintf(where + where_len, sizeof(where) - where_len,
                         " AND price >= %s", min_price);
        }
    }

    if (max_price != NULL && where_len < sizeof(where) - 1) {
        /* Validate max_price is numeric to prevent injection */
        bool valid = true;
        for (const char *p = max_price; *p && valid; p++) {
            if ((*p < '0' || *p > '9') && *p != '.') valid = false;
        }
        if (valid) {
            where_len += (size_t)snprintf(where + where_len, sizeof(where) - where_len,
                         " AND price <= %s", max_price);
        }
    }

    if (is_featured != NULL && strcmp(is_featured, "true") == 0 && where_len < sizeof(where) - 1) {
        where_len += (size_t)snprintf(where + where_len, sizeof(where) - where_len,
                     " AND is_featured = true");
    }

    if (is_sale != NULL && strcmp(is_sale, "true") == 0 && where_len < sizeof(where) - 1) {
        where_len += (size_t)snprintf(where + where_len, sizeof(where) - where_len,
                     " AND is_sale = true");
    }

    (void)where_len; /* Suppress unused warning */

    /* Build ORDER BY - use predefined safe values only */
    const char *order_by = "created_at DESC";
    if (sort != NULL) {
        if (strcmp(sort, "price_asc") == 0) {
            order_by = "price ASC";
        } else if (strcmp(sort, "price_desc") == 0) {
            order_by = "price DESC";
        } else if (strcmp(sort, "name") == 0) {
            order_by = "name ASC";
        } else if (strcmp(sort, "newest") == 0) {
            order_by = "created_at DESC";
        }
        /* Invalid sort values are ignored, keeping default */
    }

    /* Get total count */
    char count_query[512];
    snprintf(count_query, sizeof(count_query), "SELECT COUNT(*) FROM products WHERE %s", where);
    int total = db_count(conn, count_query, 0, NULL);

    /* Build pagination */
    DbPagination pag;
    db_build_pagination(&pag, page, per_page, total);

    /* Get products */
    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT id, name, slug, short_description, sku, price, compare_at_price, "
             "brand, stock_quantity, is_featured, is_new, is_sale, category_id "
             "FROM products WHERE %s ORDER BY %s LIMIT %d OFFSET %d",
             where, order_by, pag.per_page, pag.offset);

    PGresult *result = db_exec(conn, query);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    /* Build response */
    cJSON *products = cJSON_CreateArray();
    int nrows = PQntuples(result);
    for (int i = 0; i < nrows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *product = product_to_json(conn, &row, false);
        if (product != NULL) {
            cJSON_AddItemToArray(products, product);
        }
    }
    PQclear(result);
    db_pool_release(pool, conn);

    /* Build response JSON */
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "data", products);
    cJSON_AddItemToObject(response, "pagination",
                          json_create_pagination(pag.page, pag.per_page, pag.total, pag.total_pages));

    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_products_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (id_str == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Product ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check if ID is numeric or slug */
    const char *query;
    bool is_numeric = true;
    for (const char *p = id_str; *p; p++) {
        if (*p < '0' || *p > '9') {
            is_numeric = false;
            break;
        }
    }

    if (is_numeric) {
        query = "SELECT * FROM products WHERE id = $1 AND is_active = true";
    } else {
        query = "SELECT * FROM products WHERE slug = $1 AND is_active = true";
    }

    const char *params[] = { id_str };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Product not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *product = product_to_json(conn, &row, true);
    PQclear(result);
    db_pool_release(pool, conn);

    if (product == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to build response");
        return;
    }

    cJSON *response = json_create_success(product);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_products_search(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *query_str = http_request_get_query_param(req, "q");

    if (query_str == NULL || query_str[0] == '\0') {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Search query required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Escape search term */
    char *escaped = db_escape_string(conn, query_str);
    if (escaped == NULL) {
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to process query");
        return;
    }

    /* Build search query using ILIKE */
    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT id, name, slug, short_description, price, compare_at_price, brand "
             "FROM products "
             "WHERE is_active = true AND ("
             "name ILIKE '%%%s%%' OR "
             "description ILIKE '%%%s%%' OR "
             "brand ILIKE '%%%s%%' OR "
             "sku ILIKE '%%%s%%'"
             ") ORDER BY name LIMIT 50",
             escaped, escaped, escaped, escaped);
    free(escaped);

    PGresult *result = db_exec(conn, query);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Search failed");
        return;
    }

    cJSON *products = cJSON_CreateArray();
    int nrows = PQntuples(result);
    for (int i = 0; i < nrows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *product = product_to_json(conn, &row, false);
        if (product != NULL) {
            cJSON_AddItemToArray(products, product);
        }
    }
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(products);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_products_featured(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *limit_str = http_request_get_query_param(req, "limit");
    int limit = limit_str ? atoi(limit_str) : 8;
    if (limit > 50) limit = 50;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id, name, slug, short_description, price, compare_at_price, "
             "brand, is_new, is_sale FROM products "
             "WHERE is_active = true AND is_featured = true "
             "ORDER BY created_at DESC LIMIT %d", limit);

    PGresult *result = db_exec(conn, query);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *products = cJSON_CreateArray();
    int nrows = PQntuples(result);
    for (int i = 0; i < nrows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *product = product_to_json(conn, &row, false);
        if (product != NULL) {
            cJSON_AddItemToArray(products, product);
        }
    }
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(products);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_categories_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    DbPool *pool = (DbPool *)user_data;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT id, name, slug, description, image_url, parent_id "
        "FROM categories WHERE is_active = true ORDER BY sort_order, name";

    PGresult *result = db_exec(conn, query);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *categories = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(categories);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_categories_products(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *slug = http_request_get_path_param(req, "slug");

    if (slug == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Category slug required");
        return;
    }

    /* Delegate to products list with category filter */
    /* Add category to query params */
    if (req->query_param_count < HTTP_MAX_PARAMS) {
        req->query_params[req->query_param_count].name = str_dup("category");
        req->query_params[req->query_param_count].value = str_dup(slug);
        req->query_param_count++;
    }

    handler_products_list(req, resp, user_data);
}
