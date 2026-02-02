#include "middleware/middleware_analytics.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

#define SESSION_COOKIE_NAME "coldnb_session"
#define SESSION_COOKIE_MAX_AGE 86400 * 30  /* 30 days */

/* Static database pool reference */
static DbPool *g_analytics_pool = NULL;

void analytics_middleware_init(DbPool *pool) {
    g_analytics_pool = pool;
    LOG_INFO("Analytics middleware initialized");
}

char *analytics_get_session_id(HttpRequest *req) {
    /* Try to get from cookie first */
    const char *cookie_header = http_request_get_header(req, "Cookie");
    if (cookie_header != NULL) {
        /* Parse cookie header looking for our session cookie */
        const char *cookie_name = SESSION_COOKIE_NAME "=";
        const char *found = strstr(cookie_header, cookie_name);
        if (found != NULL) {
            found += strlen(cookie_name);
            const char *end = strchr(found, ';');
            size_t len = end ? (size_t)(end - found) : strlen(found);

            if (len == 36) {  /* UUID length */
                char *session_id = str_ndup(found, len);
                if (session_id != NULL && uuid_validate(session_id)) {
                    return session_id;
                }
                free(session_id);
            }
        }
    }

    /* Try X-Session-ID header */
    const char *session_header = http_request_get_header(req, "X-Session-ID");
    if (session_header != NULL && uuid_validate(session_header)) {
        return str_dup(session_header);
    }

    /* Generate new session ID */
    return coldnb_uuid_generate();
}

void analytics_set_session_cookie(HttpResponse *resp, const char *session_id) {
    if (session_id == NULL) {
        return;
    }

    char cookie_value[256];
    snprintf(cookie_value, sizeof(cookie_value),
             "%s=%s; Path=/; Max-Age=%d; HttpOnly; SameSite=Lax",
             SESSION_COOKIE_NAME, session_id, SESSION_COOKIE_MAX_AGE);

    http_response_add_header(resp, "Set-Cookie", cookie_value);
}

bool analytics_middleware_page_view(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)user_data;

    if (g_analytics_pool == NULL) {
        return true;  /* Analytics not configured, continue request */
    }

    /* Skip tracking for API endpoints that aren't page views */
    if (req->path == NULL) {
        return true;
    }

    /* Skip health checks and static assets */
    if (str_starts_with(req->path, "/health") ||
        str_starts_with(req->path, "/api/health") ||
        str_starts_with(req->path, "/favicon") ||
        str_starts_with(req->path, "/static/")) {
        return true;
    }

    /* Get session ID */
    char *session_id = analytics_get_session_id(req);
    if (session_id == NULL) {
        return true;
    }

    /* Set session cookie in response */
    analytics_set_session_cookie(resp, session_id);

    /* Get user ID if authenticated */
    const char *user_id = auth_get_user_id(req);

    /* Get referrer and user agent */
    const char *referrer = http_request_get_header(req, "Referer");
    const char *user_agent = http_request_get_header(req, "User-Agent");
    const char *ip_address = req->client_ip;

    /* Insert page view asynchronously (don't block request) */
    PGconn *conn = db_pool_acquire(g_analytics_pool);
    if (conn != NULL) {
        const char *query =
            "INSERT INTO analytics_page_views "
            "(session_id, user_id, path, referrer, user_agent, ip_address) "
            "VALUES ($1, $2, $3, $4, $5, $6::inet)";

        const char *params[] = {
            session_id,
            user_id,
            req->path,
            referrer,
            user_agent,
            ip_address
        };

        PGresult *result = db_exec_params(conn, query, 6, params);
        if (!db_result_ok(result)) {
            LOG_DEBUG("Failed to insert page view: %s", PQerrorMessage(conn));
        }
        PQclear(result);
        db_pool_release(g_analytics_pool, conn);
    }

    free(session_id);
    return true;  /* Always continue processing the request */
}

int analytics_track_product_view(const char *session_id, const char *user_id,
                                  int product_id, const char *ip_address) {
    if (g_analytics_pool == NULL) {
        return -1;
    }

    if (product_id <= 0) {
        return -1;
    }

    /* Use provided session_id or generate one */
    char *sid = NULL;
    if (session_id == NULL || session_id[0] == '\0') {
        sid = coldnb_uuid_generate();
    } else {
        sid = str_dup(session_id);
    }

    if (sid == NULL) {
        return -1;
    }

    PGconn *conn = db_pool_acquire(g_analytics_pool);
    if (conn == NULL) {
        free(sid);
        return -1;
    }

    const char *query =
        "INSERT INTO analytics_product_views (session_id, user_id, product_id) "
        "VALUES ($1, $2, $3)";

    char product_str[16];
    snprintf(product_str, sizeof(product_str), "%d", product_id);

    const char *params[] = { sid, user_id, product_str };

    PGresult *result = db_exec_params(conn, query, 3, params);

    bool success = db_result_ok(result);
    if (!success) {
        LOG_DEBUG("Failed to insert product view: %s", PQerrorMessage(conn));
    }

    PQclear(result);
    db_pool_release(g_analytics_pool, conn);
    free(sid);

    /* Suppress unused parameter warning */
    (void)ip_address;

    return success ? 0 : -1;
}
