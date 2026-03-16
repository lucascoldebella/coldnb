#ifndef COLDNB_HANDLER_USER_H
#define COLDNB_HANDLER_USER_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register user profile routes (all require authentication) */
void handler_user_register(HttpRouter *router, DbPool *pool);

/* GET /api/user/profile - Get user profile */
void handler_user_profile_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/user/profile - Update user profile */
void handler_user_profile_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/user/profile - Delete authenticated user account */
void handler_user_profile_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/user/export - GDPR data export (download all user data as JSON) */
void handler_user_data_export(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_USER_H */
