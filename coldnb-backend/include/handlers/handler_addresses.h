#ifndef COLDNB_HANDLER_ADDRESSES_H
#define COLDNB_HANDLER_ADDRESSES_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register address routes (all require authentication) */
void handler_addresses_register(HttpRouter *router, DbPool *pool);

/* GET /api/addresses - List user addresses */
void handler_addresses_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/addresses/:id - Get single address */
void handler_addresses_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/addresses - Create new address */
void handler_addresses_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/addresses/:id - Update address */
void handler_addresses_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/addresses/:id - Delete address */
void handler_addresses_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/addresses/:id/default - Set as default address */
void handler_addresses_set_default(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADDRESSES_H */
