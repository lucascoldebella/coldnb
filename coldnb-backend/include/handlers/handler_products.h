#ifndef COLDNB_HANDLER_PRODUCTS_H
#define COLDNB_HANDLER_PRODUCTS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register product routes */
void handler_products_register(HttpRouter *router, DbPool *pool);

/* GET /api/products - List products with filters and pagination */
void handler_products_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/products/:id - Get single product */
void handler_products_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/products/search - Search products */
void handler_products_search(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/products/featured - Get featured products */
void handler_products_featured(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/categories - List categories */
void handler_categories_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/categories/:slug/products - Get products by category */
void handler_categories_products(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/products/:id/recommendations - Get product recommendations */
void handler_products_recommendations(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_PRODUCTS_H */
