#ifndef COLDNB_HANDLER_CONTACT_H
#define COLDNB_HANDLER_CONTACT_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register contact routes (public) */
void handler_contact_register(HttpRouter *router, DbPool *pool);

/* POST /api/contact - Submit contact form */
void handler_contact_submit(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Register admin contact routes (admin auth required) */
void handler_admin_contact_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/contacts - List all contact submissions */
void handler_admin_contact_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/contacts/:id/read - Mark submission as read */
void handler_admin_contact_mark_read(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_CONTACT_H */
