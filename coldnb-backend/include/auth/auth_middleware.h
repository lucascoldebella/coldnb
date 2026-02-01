#ifndef COLDNB_AUTH_MIDDLEWARE_H
#define COLDNB_AUTH_MIDDLEWARE_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "auth/auth_supabase.h"
#include "db/db_connection.h"
#include <stdbool.h>

/* User context attached to request after authentication */
typedef struct {
    char *user_id;          /* User ID (UUID) */
    char *email;
    char *role;
    bool is_admin;          /* True if authenticated as admin */
} AuthContext;

/* Get auth context from request
 * Returns NULL if not authenticated */
AuthContext *auth_get_context(const HttpRequest *req);

/* Free auth context */
void auth_context_free(AuthContext *ctx);

/* Middleware that requires authentication
 * Sets auth context on request if valid
 * Returns 401 if no valid token */
bool auth_middleware_required(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Middleware that optionally authenticates
 * Sets auth context if valid token present, but allows request to continue */
bool auth_middleware_optional(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Middleware that requires admin authentication
 * Returns 401 if not authenticated, 403 if not admin */
bool auth_middleware_admin(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Get user ID from authenticated request
 * Returns NULL if not authenticated */
const char *auth_get_user_id(const HttpRequest *req);

/* Check if request is authenticated */
bool auth_is_authenticated(const HttpRequest *req);

/* Check if request is from admin */
bool auth_is_admin(const HttpRequest *req);

/* Set database pool for admin authentication
 * Must be called before using auth_middleware_admin */
void auth_middleware_set_db_pool(DbPool *pool);

#endif /* COLDNB_AUTH_MIDDLEWARE_H */
