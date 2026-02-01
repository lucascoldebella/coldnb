#ifndef COLDNB_HANDLER_WISHLIST_H
#define COLDNB_HANDLER_WISHLIST_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register wishlist routes (all require authentication) */
void handler_wishlist_register(HttpRouter *router, DbPool *pool);

/* GET /api/wishlist - List wishlist items */
void handler_wishlist_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/wishlist - Add item to wishlist */
void handler_wishlist_add(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/wishlist/:product_id - Remove item from wishlist */
void handler_wishlist_remove(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/wishlist/check/:product_id - Check if product is in wishlist */
void handler_wishlist_check(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_WISHLIST_H */
