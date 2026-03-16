#include "handlers/handler_admin_exports.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void handler_admin_exports_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/exports/orders", handler_admin_exports_orders, pool);
    ROUTE_GET(router, "/api/admin/exports/products", handler_admin_exports_products, pool);
    ROUTE_GET(router, "/api/admin/exports/customers", handler_admin_exports_customers, pool);
}

/* Escape a field for CSV: wrap in quotes if it contains comma, quote, or newline */
static void csv_append_field(char **buf, size_t *len, size_t *cap, const char *value, bool last) {
    const char *val = value ? value : "";
    bool needs_quote = (strchr(val, ',') || strchr(val, '"') || strchr(val, '\n'));

    /* Ensure capacity */
    size_t vlen = strlen(val);
    size_t needed = vlen * 2 + 4; /* worst case: every char doubled + quotes + separator */
    while (*len + needed >= *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }

    if (needs_quote) {
        (*buf)[(*len)++] = '"';
        for (size_t i = 0; i < vlen; i++) {
            if (val[i] == '"') {
                (*buf)[(*len)++] = '"'; /* escape quote with double quote */
            }
            (*buf)[(*len)++] = val[i];
        }
        (*buf)[(*len)++] = '"';
    } else {
        memcpy(*buf + *len, val, vlen);
        *len += vlen;
    }

    (*buf)[(*len)++] = last ? '\n' : ',';
    (*buf)[*len] = '\0';
}

void handler_admin_exports_orders(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *date_from = http_request_get_query_param(req, "date_from");
    const char *date_to = http_request_get_query_param(req, "date_to");

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build query with optional date filters */
    char query[1024];
    int param_count = 0;
    const char *params[2];

    if (date_from && date_to) {
        snprintf(query, sizeof(query),
            "SELECT o.order_number, o.status, o.payment_status, o.payment_method, "
            "o.subtotal, o.shipping_cost, o.discount_amount, o.total, "
            "o.shipping_name, o.shipping_city, o.shipping_state, o.shipping_postal_code, "
            "o.tracking_number, o.carrier, o.notes, "
            "COALESCE(u.email, o.guest_email) AS customer_email, "
            "COALESCE(u.full_name, o.guest_name) AS customer_name, "
            "o.created_at "
            "FROM orders o LEFT JOIN users u ON o.user_id = u.id "
            "WHERE o.created_at >= $1::date AND o.created_at < ($2::date + interval '1 day') "
            "ORDER BY o.created_at DESC");
        params[0] = date_from;
        params[1] = date_to;
        param_count = 2;
    } else {
        snprintf(query, sizeof(query),
            "SELECT o.order_number, o.status, o.payment_status, o.payment_method, "
            "o.subtotal, o.shipping_cost, o.discount_amount, o.total, "
            "o.shipping_name, o.shipping_city, o.shipping_state, o.shipping_postal_code, "
            "o.tracking_number, o.carrier, o.notes, "
            "COALESCE(u.email, o.guest_email) AS customer_email, "
            "COALESCE(u.full_name, o.guest_name) AS customer_name, "
            "o.created_at "
            "FROM orders o LEFT JOIN users u ON o.user_id = u.id "
            "ORDER BY o.created_at DESC");
    }

    PGresult *result = db_exec_params(conn, query, param_count, param_count > 0 ? params : NULL);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    int cols = PQnfields(result);

    /* Build CSV */
    size_t cap = 4096;
    size_t len = 0;
    char *csv = malloc(cap);
    csv[0] = '\0';

    /* Header row */
    const char *headers[] = {
        "Order Number", "Status", "Payment Status", "Payment Method",
        "Subtotal", "Shipping", "Discount", "Total",
        "Shipping Name", "City", "State", "Postal Code",
        "Tracking", "Carrier", "Notes",
        "Customer Email", "Customer Name", "Created At"
    };
    for (int c = 0; c < cols && c < 18; c++) {
        csv_append_field(&csv, &len, &cap, headers[c], c == cols - 1 || c == 17);
    }

    /* Data rows */
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols && c < 18; c++) {
            const char *val = PQgetvalue(result, r, c);
            csv_append_field(&csv, &len, &cap, PQgetisnull(result, r, c) ? "" : val,
                           c == cols - 1 || c == 17);
        }
    }

    PQclear(result);

    http_response_add_header(resp, "Content-Disposition", "attachment; filename=\"orders.csv\"");
    http_response_set_content_type(resp, "text/csv; charset=utf-8");
    http_response_set_status(resp, HTTP_STATUS_OK);
    http_response_set_body(resp, csv, len);
    free(csv);

    LOG_INFO("Admin exported %d orders as CSV", rows);
}

void handler_admin_exports_products(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT p.id, p.name, p.sku, p.slug, p.price, p.compare_at_price, "
        "p.stock_quantity, p.is_active, p.is_featured, p.is_new, p.is_sale, "
        "p.brand, c.name AS category, p.created_at "
        "FROM products p "
        "LEFT JOIN categories c ON p.category_id = c.id "
        "ORDER BY p.id";

    PGresult *result = db_exec(conn, query);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    int cols = PQnfields(result);

    size_t cap = 4096;
    size_t len = 0;
    char *csv = malloc(cap);
    csv[0] = '\0';

    const char *headers[] = {
        "ID", "Name", "SKU", "Slug", "Price", "Compare At Price",
        "Stock", "Active", "Featured", "New", "Sale",
        "Brand", "Category", "Created At"
    };
    for (int c = 0; c < cols && c < 14; c++) {
        csv_append_field(&csv, &len, &cap, headers[c], c == cols - 1 || c == 13);
    }

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols && c < 14; c++) {
            const char *val = PQgetvalue(result, r, c);
            csv_append_field(&csv, &len, &cap, PQgetisnull(result, r, c) ? "" : val,
                           c == cols - 1 || c == 13);
        }
    }

    PQclear(result);

    http_response_add_header(resp, "Content-Disposition", "attachment; filename=\"products.csv\"");
    http_response_set_content_type(resp, "text/csv; charset=utf-8");
    http_response_set_status(resp, HTTP_STATUS_OK);
    http_response_set_body(resp, csv, len);
    free(csv);

    LOG_INFO("Admin exported %d products as CSV", rows);
}

void handler_admin_exports_customers(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT u.id, u.email, u.full_name, u.phone, u.is_active, u.email_verified, "
        "u.created_at, "
        "COUNT(o.id) AS order_count, "
        "COALESCE(SUM(o.total), 0) AS total_spent "
        "FROM users u "
        "LEFT JOIN orders o ON o.user_id = u.id "
        "GROUP BY u.id "
        "ORDER BY u.created_at DESC";

    PGresult *result = db_exec(conn, query);
    db_pool_release(pool, conn);

    if (!db_result_ok(result)) {
        PQclear(result);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    int rows = PQntuples(result);
    int cols = PQnfields(result);

    size_t cap = 4096;
    size_t len = 0;
    char *csv = malloc(cap);
    csv[0] = '\0';

    const char *headers[] = {
        "ID", "Email", "Full Name", "Phone", "Active", "Email Verified",
        "Created At", "Order Count", "Total Spent"
    };
    for (int c = 0; c < cols && c < 9; c++) {
        csv_append_field(&csv, &len, &cap, headers[c], c == cols - 1 || c == 8);
    }

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols && c < 9; c++) {
            const char *val = PQgetvalue(result, r, c);
            csv_append_field(&csv, &len, &cap, PQgetisnull(result, r, c) ? "" : val,
                           c == cols - 1 || c == 8);
        }
    }

    PQclear(result);

    http_response_add_header(resp, "Content-Disposition", "attachment; filename=\"customers.csv\"");
    http_response_set_content_type(resp, "text/csv; charset=utf-8");
    http_response_set_status(resp, HTTP_STATUS_OK);
    http_response_set_body(resp, csv, len);
    free(csv);

    LOG_INFO("Admin exported %d customers as CSV", rows);
}
