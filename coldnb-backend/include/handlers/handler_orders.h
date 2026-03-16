#ifndef COLDNB_HANDLER_ORDERS_H
#define COLDNB_HANDLER_ORDERS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register order routes (all require authentication) */
void handler_orders_register(HttpRouter *router, DbPool *pool);

/* Register public order tracking route (NO auth required) */
void handler_orders_track_register(HttpRouter *router, DbPool *pool);

/* POST /api/orders - Create order from cart */
void handler_orders_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/orders - List user orders (paginated) */
void handler_orders_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/orders/:id - Get order details */
void handler_orders_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/orders/track - Public order tracking (order_number + email) */
void handler_orders_track(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/orders/:id/cancel - Customer-initiated order cancellation */
void handler_orders_cancel(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ORDERS_H */
