#ifndef COLDNB_HANDLER_ADMIN_NAVIGATION_H
#define COLDNB_HANDLER_ADMIN_NAVIGATION_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register navigation admin routes and public endpoint */
void handler_admin_navigation_register(HttpRouter *router, DbPool *pool);

/* GET /api/navigation - Public endpoint returning all menus with nested groups/items */
void handler_navigation_public(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Menus */
void handler_admin_nav_menus_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_menus_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_menus_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_menus_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Groups */
void handler_admin_nav_groups_list(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_groups_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_groups_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_groups_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Items */
void handler_admin_nav_items_create(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_items_update(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_items_delete(HttpRequest *req, HttpResponse *resp, void *user_data);
void handler_admin_nav_items_reorder(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_NAVIGATION_H */
