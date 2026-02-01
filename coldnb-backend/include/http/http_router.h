#ifndef COLDNB_HTTP_ROUTER_H
#define COLDNB_HTTP_ROUTER_H

#include "http/http_request.h"
#include "http/http_response.h"
#include <stdbool.h>

/* Forward declaration */
typedef struct HttpRouter HttpRouter;

/* Request handler function type */
typedef void (*HttpHandler)(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Middleware function type
 * Returns true to continue to next middleware/handler, false to stop */
typedef bool (*HttpMiddleware)(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Create a new router */
HttpRouter *http_router_create(void);

/* Free router */
void http_router_free(HttpRouter *router);

/* Register a route
 * Pattern can include path parameters like /api/products/:id
 * user_data is passed to handler */
int http_router_add(HttpRouter *router, HttpMethod method, const char *pattern,
                    HttpHandler handler, void *user_data);

/* Convenience macros for route registration */
#define ROUTE_GET(router, pattern, handler, data) \
    http_router_add(router, HTTP_METHOD_GET, pattern, handler, data)

#define ROUTE_POST(router, pattern, handler, data) \
    http_router_add(router, HTTP_METHOD_POST, pattern, handler, data)

#define ROUTE_PUT(router, pattern, handler, data) \
    http_router_add(router, HTTP_METHOD_PUT, pattern, handler, data)

#define ROUTE_DELETE(router, pattern, handler, data) \
    http_router_add(router, HTTP_METHOD_DELETE, pattern, handler, data)

#define ROUTE_PATCH(router, pattern, handler, data) \
    http_router_add(router, HTTP_METHOD_PATCH, pattern, handler, data)

/* Register global middleware (runs before any route handler)
 * Middlewares run in the order they are added */
int http_router_use(HttpRouter *router, HttpMiddleware middleware, void *user_data);

/* Register middleware for specific path prefix
 * Pattern uses the same syntax as routes */
int http_router_use_path(HttpRouter *router, const char *prefix,
                         HttpMiddleware middleware, void *user_data);

/* Find and execute matching route
 * Returns true if a route was found and executed */
bool http_router_handle(HttpRouter *router, HttpRequest *req, HttpResponse *resp);

/* Set handler for 404 Not Found */
void http_router_set_not_found(HttpRouter *router, HttpHandler handler, void *user_data);

/* Set handler for 405 Method Not Allowed */
void http_router_set_method_not_allowed(HttpRouter *router, HttpHandler handler, void *user_data);

/* Group routes under a prefix
 * Returns a router context for adding routes with the prefix */
typedef struct {
    HttpRouter *router;
    char *prefix;
} HttpRouteGroup;

HttpRouteGroup *http_router_group(HttpRouter *router, const char *prefix);
void http_route_group_free(HttpRouteGroup *group);

/* Add route to group (prefix is automatically prepended) */
int http_route_group_add(HttpRouteGroup *group, HttpMethod method, const char *pattern,
                         HttpHandler handler, void *user_data);

/* Print registered routes (for debugging) */
void http_router_print_routes(const HttpRouter *router);

#endif /* COLDNB_HTTP_ROUTER_H */
