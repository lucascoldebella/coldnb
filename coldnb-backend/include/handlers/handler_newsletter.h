#ifndef COLDNB_HANDLER_NEWSLETTER_H
#define COLDNB_HANDLER_NEWSLETTER_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register newsletter routes (public) */
void handler_newsletter_register(HttpRouter *router, DbPool *pool);

/* POST /api/newsletter/subscribe - Subscribe to newsletter */
void handler_newsletter_subscribe(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/newsletter/unsubscribe - Unsubscribe from newsletter */
void handler_newsletter_unsubscribe(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_NEWSLETTER_H */
