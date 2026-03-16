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

/* GET /api/email/unsubscribe?email=xxx&token=xxx - One-click unsubscribe from email link */
void handler_email_unsubscribe(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Register admin newsletter routes (admin auth required) */
void handler_admin_newsletter_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/newsletter/subscribers - List all subscribers */
void handler_admin_newsletter_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/newsletter/subscribers/:id - Delete a subscriber */
void handler_admin_newsletter_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_NEWSLETTER_H */
