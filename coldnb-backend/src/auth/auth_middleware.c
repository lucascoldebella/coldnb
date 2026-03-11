#include "auth/auth_middleware.h"
#include "auth/auth_supabase.h"
#include "services/svc_admin_auth.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

/* Database pool for admin auth (set during initialization) */
static DbPool *g_admin_db_pool = NULL;

/* Set database pool for admin authentication */
void auth_middleware_set_db_pool(DbPool *pool) {
    g_admin_db_pool = pool;
}

/* Key for storing auth context in request user_data */
#define AUTH_CONTEXT_KEY "auth_context"

AuthContext *auth_get_context(const HttpRequest *req) {
    if (req == NULL || req->user_data == NULL) {
        return NULL;
    }
    return (AuthContext *)req->user_data;
}

void auth_context_free(AuthContext *ctx) {
    if (ctx == NULL) {
        return;
    }
    free(ctx->user_id);
    free(ctx->email);
    free(ctx->full_name);
    free(ctx->phone);
    free(ctx->role);
    free(ctx);
}

/* Attempt to authenticate from bearer token */
static AuthContext *try_authenticate(HttpRequest *req) {
    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        return NULL;
    }

    SupabaseUser *user = supabase_validate_token(token);
    if (user == NULL) {
        LOG_DEBUG("Token validation failed");
        return NULL;
    }

    AuthContext *ctx = calloc(1, sizeof(AuthContext));
    if (ctx == NULL) {
        supabase_user_free(user);
        return NULL;
    }

    ctx->user_id = user->user_id ? str_dup(user->user_id) : NULL;
    ctx->email = user->email ? str_dup(user->email) : NULL;
    ctx->full_name = user->full_name ? str_dup(user->full_name) : NULL;
    ctx->phone = user->phone ? str_dup(user->phone) : NULL;
    ctx->role = user->role ? str_dup(user->role) : NULL;
    ctx->email_verified = user->email_verified;
    ctx->is_admin = false;  /* User auth, not admin */

    supabase_user_free(user);
    return ctx;
}

/* Generic cleanup wrapper for AuthContext */
static void auth_context_free_wrapper(void *ptr) {
    auth_context_free((AuthContext *)ptr);
}

bool auth_middleware_required(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)user_data;

    AuthContext *ctx = try_authenticate(req);
    if (ctx == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return false;
    }

    /* Store context in request with cleanup function */
    req->user_data = ctx;
    req->user_data_free = auth_context_free_wrapper;
    LOG_DEBUG("Authenticated user: %s", ctx->user_id);

    return true;
}

bool auth_middleware_optional(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)resp;
    (void)user_data;

    /* Try to authenticate, but don't fail if no token */
    AuthContext *ctx = try_authenticate(req);
    if (ctx != NULL) {
        req->user_data = ctx;
        req->user_data_free = auth_context_free_wrapper;
        LOG_DEBUG("Optionally authenticated user: %s", ctx->user_id);
    }

    return true;  /* Always continue */
}

bool auth_middleware_admin(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Use global pool if not passed as user_data */
    if (pool == NULL) {
        pool = g_admin_db_pool;
    }

    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Admin authentication required");
        return false;
    }

    /* Validate admin JWT token */
    if (pool == NULL) {
        LOG_ERROR("No database pool available for admin auth");
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Server configuration error");
        return false;
    }

    AdminUser *admin = admin_auth_validate_token(pool, token);
    if (admin == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Invalid or expired admin token");
        return false;
    }

    /* Check if admin is active */
    if (!admin->is_active) {
        admin_user_free(admin);
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin account disabled");
        return false;
    }

    AuthContext *ctx = calloc(1, sizeof(AuthContext));
    if (ctx == NULL) {
        admin_user_free(admin);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Internal error");
        return false;
    }

    ctx->user_id = admin->id ? str_dup(admin->id) : NULL;
    ctx->email = admin->email ? str_dup(admin->email) : NULL;
    ctx->role = admin->role ? str_dup(admin->role) : NULL;
    ctx->is_admin = true;

    admin_user_free(admin);
    req->user_data = ctx;
    req->user_data_free = auth_context_free_wrapper;

    LOG_DEBUG("Authenticated admin: %s (role: %s)", ctx->email, ctx->role);
    return true;
}

const char *auth_get_user_id(const HttpRequest *req) {
    AuthContext *ctx = auth_get_context(req);
    if (ctx == NULL) {
        return NULL;
    }
    return ctx->user_id;
}

bool auth_is_authenticated(const HttpRequest *req) {
    return auth_get_context(req) != NULL;
}

bool auth_is_admin(const HttpRequest *req) {
    AuthContext *ctx = auth_get_context(req);
    if (ctx == NULL) {
        return false;
    }
    return ctx->is_admin;
}
