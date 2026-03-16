#ifndef COLDNB_HANDLER_DISCOUNTS_H
#define COLDNB_HANDLER_DISCOUNTS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register discount routes (public + admin) */
void handler_discounts_register(HttpRouter *router, DbPool *pool);

/* GET /api/discount-codes/check?code=XXX - Validate a discount code (public) */
void handler_discounts_check(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/discounts - List all discount codes */
void handler_admin_discounts_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/discounts - Create a discount code */
void handler_admin_discounts_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/discounts/:id - Update a discount code */
void handler_admin_discounts_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/discounts/:id - Delete a discount code */
void handler_admin_discounts_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_DISCOUNTS_H */
