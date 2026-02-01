#ifndef COLDNB_HANDLER_ADMIN_ANALYTICS_H
#define COLDNB_HANDLER_ADMIN_ANALYTICS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register admin analytics routes (all require admin auth) */
void handler_admin_analytics_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/analytics/dashboard - Dashboard overview */
void handler_admin_analytics_dashboard(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/analytics/sales - Sales analytics */
void handler_admin_analytics_sales(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/analytics/products - Top products analytics */
void handler_admin_analytics_products(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/analytics/traffic - Traffic analytics */
void handler_admin_analytics_traffic(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_ANALYTICS_H */
