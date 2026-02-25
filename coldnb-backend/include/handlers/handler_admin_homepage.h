#ifndef COLDNB_HANDLER_ADMIN_HOMEPAGE_H
#define COLDNB_HANDLER_ADMIN_HOMEPAGE_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register homepage admin routes and public endpoint */
void handler_admin_homepage_register(HttpRouter *router, DbPool *pool);

/* GET /api/homepage - Public endpoint returning all homepage content */
void handler_homepage_public(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Hero Slides */
void handler_admin_hero_slides_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_hero_slides_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_hero_slides_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_hero_slides_delete(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_hero_slides_reorder(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Banners */
void handler_admin_banners_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_banners_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_banners_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_banners_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Sections */
void handler_admin_sections_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_sections_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_sections_products(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Campaigns */
void handler_admin_campaigns_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_campaigns_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_campaigns_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_campaigns_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_HOMEPAGE_H */
