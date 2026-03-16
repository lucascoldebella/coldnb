#ifndef COLDNB_HANDLER_ADMIN_EXPORTS_H
#define COLDNB_HANDLER_ADMIN_EXPORTS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register admin export routes (all require admin auth) */
void handler_admin_exports_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/exports/orders - Export orders as CSV */
void handler_admin_exports_orders(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/exports/products - Export products as CSV */
void handler_admin_exports_products(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/exports/customers - Export customers as CSV */
void handler_admin_exports_customers(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_EXPORTS_H */
