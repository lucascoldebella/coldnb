#ifndef COLDNB_HANDLER_SHIPPING_H
#define COLDNB_HANDLER_SHIPPING_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register shipping routes (public + admin) */
void handler_shipping_register(HttpRouter *router, DbPool *pool);

/* GET /api/shipping/calculate?cep=XXXXX-XXX - Calculate shipping cost */
void handler_shipping_calculate(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/shipping/zones - List all shipping zones */
void handler_admin_shipping_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/shipping/zones - Create shipping zone */
void handler_admin_shipping_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/shipping/zones/:id - Update shipping zone */
void handler_admin_shipping_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/shipping/zones/:id - Delete shipping zone */
void handler_admin_shipping_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_SHIPPING_H */
