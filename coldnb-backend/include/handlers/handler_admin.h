#ifndef COLDNB_HANDLER_ADMIN_H
#define COLDNB_HANDLER_ADMIN_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register admin authentication routes */
void handler_admin_register(HttpRouter *router, DbPool *pool);

/* POST /api/admin/login - Admin login */
void handler_admin_login(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/logout - Admin logout */
void handler_admin_logout(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/me - Get current admin info */
void handler_admin_me(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_H */
