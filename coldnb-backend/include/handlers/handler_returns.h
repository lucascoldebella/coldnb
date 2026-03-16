#ifndef COLDNB_HANDLER_RETURNS_H
#define COLDNB_HANDLER_RETURNS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register customer return routes (auth required) */
void handler_returns_register(HttpRouter *router, DbPool *pool);

/* POST /api/returns - Submit a return request */
void handler_returns_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/returns - List user's return requests */
void handler_returns_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/returns/:id - Get a single return request */
void handler_returns_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Register admin return routes (admin auth required) */
void handler_admin_returns_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/returns - List all return requests */
void handler_admin_returns_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/returns/:id/status - Update return status */
void handler_admin_returns_update(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_RETURNS_H */
