#ifndef COLDNB_MIDDLEWARE_ANALYTICS_H
#define COLDNB_MIDDLEWARE_ANALYTICS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "db/db_connection.h"
#include <stdbool.h>

/* Initialize analytics middleware with database pool
 * Must be called before using middleware functions */
void analytics_middleware_init(DbPool *pool);

/* Page view tracking middleware
 * Tracks: path, session_id, user_id, referrer, IP address
 * Always returns true (non-blocking) */
bool analytics_middleware_page_view(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Track product view
 * Called from product detail handler
 * Returns 0 on success, -1 on error */
int analytics_track_product_view(const char *session_id, const char *user_id,
                                  int product_id, const char *ip_address);

/* Get or create session ID from cookie/header
 * Returns session ID (caller must free) or NULL */
char *analytics_get_session_id(HttpRequest *req);

/* Set session ID cookie in response */
void analytics_set_session_cookie(HttpResponse *resp, const char *session_id);

#endif /* COLDNB_MIDDLEWARE_ANALYTICS_H */
