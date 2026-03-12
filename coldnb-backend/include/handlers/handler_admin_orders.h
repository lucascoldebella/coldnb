#ifndef COLDNB_HANDLER_ADMIN_ORDERS_H
#define COLDNB_HANDLER_ADMIN_ORDERS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register admin order routes (all require admin auth) */
void handler_admin_orders_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/orders - List all orders (paginated, filterable) */
void handler_admin_orders_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/orders/:id - Get order details */
void handler_admin_orders_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/orders/:id/status - Update order status */
void handler_admin_orders_update_status(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/orders/:id/tracking - Update tracking info */
void handler_admin_orders_update_tracking(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/users - List users */
void handler_admin_users_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/users/:id - Get user details */
void handler_admin_users_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/users/:id - Update user */
void handler_admin_users_update(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_ORDERS_H */
