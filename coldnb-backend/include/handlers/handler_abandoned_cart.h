#ifndef COLDNB_HANDLER_ABANDONED_CART_H
#define COLDNB_HANDLER_ABANDONED_CART_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register abandoned cart routes (admin) */
void handler_abandoned_cart_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/abandoned-carts - List abandoned carts with stats */
void handler_admin_abandoned_carts_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/abandoned-carts/send - Send recovery emails */
void handler_admin_abandoned_carts_send(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ABANDONED_CART_H */
