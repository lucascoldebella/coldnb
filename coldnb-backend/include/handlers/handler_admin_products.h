#ifndef COLDNB_HANDLER_ADMIN_PRODUCTS_H
#define COLDNB_HANDLER_ADMIN_PRODUCTS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register admin product routes (all require admin auth) */
void handler_admin_products_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/products - List all products */
void handler_admin_products_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/products/:id - Get product details */
void handler_admin_products_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/products - Create product */
void handler_admin_products_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/products/:id - Update product */
void handler_admin_products_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/products/:id - Delete product (soft delete) */
void handler_admin_products_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/products/:id/images - Add product image */
void handler_admin_products_add_image(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/products/:id/images/:image_id - Remove product image */
void handler_admin_products_remove_image(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_PRODUCTS_H */
