#ifndef COLDNB_HANDLER_CART_H
#define COLDNB_HANDLER_CART_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register cart routes (all require authentication) */
void handler_cart_register(HttpRouter *router, DbPool *pool);

/* GET /api/cart - Get user's cart */
void handler_cart_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/cart - Add item to cart */
void handler_cart_add(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/cart/:id - Update cart item quantity */
void handler_cart_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/cart/:id - Remove item from cart */
void handler_cart_remove(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/cart - Clear entire cart */
void handler_cart_clear(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/cart/count - Get cart item count */
void handler_cart_count(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_CART_H */
