#include "handlers/handler_admin_analytics.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_admin_analytics_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/analytics/dashboard", handler_admin_analytics_dashboard, pool);
    ROUTE_GET(router, "/api/admin/analytics/sales", handler_admin_analytics_sales, pool);
    ROUTE_GET(router, "/api/admin/analytics/products", handler_admin_analytics_products, pool);
    ROUTE_GET(router, "/api/admin/analytics/traffic", handler_admin_analytics_traffic, pool);
}

void handler_admin_analytics_dashboard(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    cJSON *data = cJSON_CreateObject();

    /* Total revenue (all time) */
    const char *revenue_query =
        "SELECT COALESCE(SUM(total), 0) AS total_revenue, COUNT(*) AS total_orders "
        "FROM orders WHERE payment_status = 'paid'";
    PGresult *revenue_result = db_exec(conn, revenue_query);
    if (db_result_ok(revenue_result) && db_result_has_rows(revenue_result)) {
        DbRow row = { .result = revenue_result, .row = 0 };
        cJSON_AddNumberToObject(data, "total_revenue", db_row_get_double(&row, "total_revenue"));
        cJSON_AddNumberToObject(data, "total_orders", db_row_get_int(&row, "total_orders"));
    }
    PQclear(revenue_result);

    /* Today's stats */
    const char *today_query =
        "SELECT COALESCE(SUM(total), 0) AS today_revenue, COUNT(*) AS today_orders "
        "FROM orders WHERE payment_status = 'paid' AND DATE(created_at) = CURRENT_DATE";
    PGresult *today_result = db_exec(conn, today_query);
    if (db_result_ok(today_result) && db_result_has_rows(today_result)) {
        DbRow row = { .result = today_result, .row = 0 };
        cJSON_AddNumberToObject(data, "today_revenue", db_row_get_double(&row, "today_revenue"));
        cJSON_AddNumberToObject(data, "today_orders", db_row_get_int(&row, "today_orders"));
    }
    PQclear(today_result);

    /* This month's stats */
    const char *month_query =
        "SELECT COALESCE(SUM(total), 0) AS month_revenue, COUNT(*) AS month_orders "
        "FROM orders WHERE payment_status = 'paid' "
        "AND DATE_TRUNC('month', created_at) = DATE_TRUNC('month', CURRENT_DATE)";
    PGresult *month_result = db_exec(conn, month_query);
    if (db_result_ok(month_result) && db_result_has_rows(month_result)) {
        DbRow row = { .result = month_result, .row = 0 };
        cJSON_AddNumberToObject(data, "month_revenue", db_row_get_double(&row, "month_revenue"));
        cJSON_AddNumberToObject(data, "month_orders", db_row_get_int(&row, "month_orders"));
    }
    PQclear(month_result);

    /* Total customers */
    const char *customers_query = "SELECT COUNT(*) FROM users WHERE is_active = true";
    int total_customers = db_count(conn, customers_query, 0, NULL);
    cJSON_AddNumberToObject(data, "total_customers", total_customers);

    /* New customers this month */
    const char *new_customers_query =
        "SELECT COUNT(*) FROM users WHERE is_active = true "
        "AND DATE_TRUNC('month', created_at) = DATE_TRUNC('month', CURRENT_DATE)";
    int new_customers = db_count(conn, new_customers_query, 0, NULL);
    cJSON_AddNumberToObject(data, "new_customers_this_month", new_customers);

    /* Total products */
    const char *products_query = "SELECT COUNT(*) FROM products WHERE is_active = true";
    int total_products = db_count(conn, products_query, 0, NULL);
    cJSON_AddNumberToObject(data, "total_products", total_products);

    /* Low stock products */
    const char *low_stock_query =
        "SELECT COUNT(*) FROM products WHERE is_active = true AND stock_quantity <= low_stock_threshold";
    int low_stock = db_count(conn, low_stock_query, 0, NULL);
    cJSON_AddNumberToObject(data, "low_stock_products", low_stock);

    /* Pending orders */
    const char *pending_query = "SELECT COUNT(*) FROM orders WHERE status = 'pending'";
    int pending_orders = db_count(conn, pending_query, 0, NULL);
    cJSON_AddNumberToObject(data, "pending_orders", pending_orders);

    /* Orders by status */
    const char *status_query =
        "SELECT status, COUNT(*) AS count FROM orders GROUP BY status ORDER BY count DESC";
    PGresult *status_result = db_exec(conn, status_query);
    if (db_result_ok(status_result)) {
        cJSON_AddItemToObject(data, "orders_by_status", db_result_to_json(status_result));
    }
    PQclear(status_result);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_analytics_sales(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    /* Period: day, week, month, year */
    const char *period = http_request_get_query_param(req, "period");
    if (period == NULL) period = "month";

    /* Days to look back */
    const char *days_str = http_request_get_query_param(req, "days");
    int days = days_str ? atoi(days_str) : 30;
    if (days < 1) days = 30;
    if (days > 365) days = 365;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    cJSON *data = cJSON_CreateObject();

    /* Sales over time */
    char sales_query[1024];
    const char *trunc_period;

    if (strcmp(period, "day") == 0) {
        trunc_period = "hour";
    } else if (strcmp(period, "week") == 0) {
        trunc_period = "day";
    } else if (strcmp(period, "year") == 0) {
        trunc_period = "month";
    } else {
        trunc_period = "day";
    }

    snprintf(sales_query, sizeof(sales_query),
             "SELECT DATE_TRUNC('%s', created_at) AS period, "
             "COALESCE(SUM(total), 0) AS revenue, COUNT(*) AS orders "
             "FROM orders "
             "WHERE payment_status = 'paid' AND created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY period ORDER BY period",
             trunc_period, days);

    PGresult *sales_result = db_exec(conn, sales_query);
    if (db_result_ok(sales_result)) {
        cJSON_AddItemToObject(data, "sales_over_time", db_result_to_json(sales_result));
    }
    PQclear(sales_result);

    /* Total for period */
    char total_query[512];
    snprintf(total_query, sizeof(total_query),
             "SELECT COALESCE(SUM(total), 0) AS revenue, COUNT(*) AS orders, "
             "COALESCE(AVG(total), 0) AS average_order "
             "FROM orders WHERE payment_status = 'paid' AND created_at >= NOW() - INTERVAL '%d days'",
             days);

    PGresult *total_result = db_exec(conn, total_query);
    if (db_result_ok(total_result) && db_result_has_rows(total_result)) {
        DbRow row = { .result = total_result, .row = 0 };
        cJSON_AddNumberToObject(data, "period_revenue", db_row_get_double(&row, "revenue"));
        cJSON_AddNumberToObject(data, "period_orders", db_row_get_int(&row, "orders"));
        cJSON_AddNumberToObject(data, "average_order_value", db_row_get_double(&row, "average_order"));
    }
    PQclear(total_result);

    /* Payment methods breakdown */
    char payment_query[512];
    snprintf(payment_query, sizeof(payment_query),
             "SELECT payment_method, COUNT(*) AS count, COALESCE(SUM(total), 0) AS revenue "
             "FROM orders WHERE payment_status = 'paid' AND created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY payment_method ORDER BY revenue DESC",
             days);

    PGresult *payment_result = db_exec(conn, payment_query);
    if (db_result_ok(payment_result)) {
        cJSON_AddItemToObject(data, "payment_methods", db_result_to_json(payment_result));
    }
    PQclear(payment_result);

    db_pool_release(pool, conn);

    cJSON_AddNumberToObject(data, "days", days);
    cJSON_AddStringToObject(data, "period", period);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_analytics_products(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *days_str = http_request_get_query_param(req, "days");
    int days = days_str ? atoi(days_str) : 30;
    if (days < 1) days = 30;
    if (days > 365) days = 365;

    const char *limit_str = http_request_get_query_param(req, "limit");
    int limit = limit_str ? atoi(limit_str) : 10;
    if (limit < 1) limit = 10;
    if (limit > 50) limit = 50;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    cJSON *data = cJSON_CreateObject();

    /* Top selling products */
    char top_query[1024];
    snprintf(top_query, sizeof(top_query),
             "SELECT oi.product_id, oi.product_name, "
             "SUM(oi.quantity) AS total_quantity, "
             "SUM(oi.total_price) AS total_revenue, "
             "COUNT(DISTINCT oi.order_id) AS order_count "
             "FROM order_items oi "
             "JOIN orders o ON oi.order_id = o.id "
             "WHERE o.payment_status = 'paid' AND o.created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY oi.product_id, oi.product_name "
             "ORDER BY total_revenue DESC "
             "LIMIT %d",
             days, limit);

    PGresult *top_result = db_exec(conn, top_query);
    if (db_result_ok(top_result)) {
        cJSON_AddItemToObject(data, "top_selling", db_result_to_json(top_result));
    }
    PQclear(top_result);

    /* Most viewed products */
    char views_query[512];
    snprintf(views_query, sizeof(views_query),
             "SELECT pv.product_id, p.name AS product_name, COUNT(*) AS view_count "
             "FROM analytics_product_views pv "
             "JOIN products p ON pv.product_id = p.id "
             "WHERE pv.created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY pv.product_id, p.name "
             "ORDER BY view_count DESC "
             "LIMIT %d",
             days, limit);

    PGresult *views_result = db_exec(conn, views_query);
    if (db_result_ok(views_result)) {
        cJSON_AddItemToObject(data, "most_viewed", db_result_to_json(views_result));
    }
    PQclear(views_result);

    /* Low stock products */
    char low_stock_query[512];
    snprintf(low_stock_query, sizeof(low_stock_query),
             "SELECT id, name, sku, stock_quantity, low_stock_threshold "
             "FROM products "
             "WHERE is_active = true AND stock_quantity <= low_stock_threshold "
             "ORDER BY stock_quantity ASC "
             "LIMIT %d",
             limit);

    PGresult *low_stock_result = db_exec(conn, low_stock_query);
    if (db_result_ok(low_stock_result)) {
        cJSON_AddItemToObject(data, "low_stock", db_result_to_json(low_stock_result));
    }
    PQclear(low_stock_result);

    /* Category breakdown */
    char category_query[512];
    snprintf(category_query, sizeof(category_query),
             "SELECT c.name AS category, COUNT(DISTINCT oi.product_id) AS products_sold, "
             "SUM(oi.quantity) AS total_quantity, SUM(oi.total_price) AS revenue "
             "FROM order_items oi "
             "JOIN orders o ON oi.order_id = o.id "
             "JOIN products p ON oi.product_id = p.id "
             "LEFT JOIN categories c ON p.category_id = c.id "
             "WHERE o.payment_status = 'paid' AND o.created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY c.name "
             "ORDER BY revenue DESC",
             days);

    PGresult *category_result = db_exec(conn, category_query);
    if (db_result_ok(category_result)) {
        cJSON_AddItemToObject(data, "by_category", db_result_to_json(category_result));
    }
    PQclear(category_result);

    db_pool_release(pool, conn);

    cJSON_AddNumberToObject(data, "days", days);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_analytics_traffic(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *days_str = http_request_get_query_param(req, "days");
    int days = days_str ? atoi(days_str) : 7;
    if (days < 1) days = 7;
    if (days > 90) days = 90;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    cJSON *data = cJSON_CreateObject();

    /* Page views over time */
    char views_query[512];
    snprintf(views_query, sizeof(views_query),
             "SELECT DATE(created_at) AS date, COUNT(*) AS page_views, "
             "COUNT(DISTINCT session_id) AS unique_sessions "
             "FROM analytics_page_views "
             "WHERE created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY DATE(created_at) "
             "ORDER BY date",
             days);

    PGresult *views_result = db_exec(conn, views_query);
    if (db_result_ok(views_result)) {
        cJSON_AddItemToObject(data, "traffic_over_time", db_result_to_json(views_result));
    }
    PQclear(views_result);

    /* Top pages */
    char pages_query[512];
    snprintf(pages_query, sizeof(pages_query),
             "SELECT path, COUNT(*) AS views "
             "FROM analytics_page_views "
             "WHERE created_at >= NOW() - INTERVAL '%d days' "
             "GROUP BY path "
             "ORDER BY views DESC "
             "LIMIT 20",
             days);

    PGresult *pages_result = db_exec(conn, pages_query);
    if (db_result_ok(pages_result)) {
        cJSON_AddItemToObject(data, "top_pages", db_result_to_json(pages_result));
    }
    PQclear(pages_result);

    /* Top referrers */
    char referrers_query[512];
    snprintf(referrers_query, sizeof(referrers_query),
             "SELECT referrer, COUNT(*) AS visits "
             "FROM analytics_page_views "
             "WHERE created_at >= NOW() - INTERVAL '%d days' "
             "AND referrer IS NOT NULL AND referrer != '' "
             "GROUP BY referrer "
             "ORDER BY visits DESC "
             "LIMIT 10",
             days);

    PGresult *referrers_result = db_exec(conn, referrers_query);
    if (db_result_ok(referrers_result)) {
        cJSON_AddItemToObject(data, "top_referrers", db_result_to_json(referrers_result));
    }
    PQclear(referrers_result);

    /* Summary stats */
    char summary_query[512];
    snprintf(summary_query, sizeof(summary_query),
             "SELECT COUNT(*) AS total_page_views, "
             "COUNT(DISTINCT session_id) AS unique_sessions, "
             "COUNT(DISTINCT user_id) FILTER (WHERE user_id IS NOT NULL) AS logged_in_users "
             "FROM analytics_page_views "
             "WHERE created_at >= NOW() - INTERVAL '%d days'",
             days);

    PGresult *summary_result = db_exec(conn, summary_query);
    if (db_result_ok(summary_result) && db_result_has_rows(summary_result)) {
        DbRow row = { .result = summary_result, .row = 0 };
        cJSON_AddNumberToObject(data, "total_page_views", db_row_get_int(&row, "total_page_views"));
        cJSON_AddNumberToObject(data, "unique_sessions", db_row_get_int(&row, "unique_sessions"));
        cJSON_AddNumberToObject(data, "logged_in_users", db_row_get_int(&row, "logged_in_users"));
    }
    PQclear(summary_result);

    db_pool_release(pool, conn);

    cJSON_AddNumberToObject(data, "days", days);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
