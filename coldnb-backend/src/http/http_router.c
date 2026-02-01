#include "http/http_router.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

/* Maximum number of routes and middlewares */
#define MAX_ROUTES 256
#define MAX_MIDDLEWARES 32
#define MAX_PATH_SEGMENTS 32

/* Route entry */
typedef struct {
    HttpMethod method;
    char *pattern;
    char **segments;        /* Split pattern segments */
    size_t segment_count;
    bool *is_param;         /* Which segments are parameters */
    char **param_names;     /* Parameter names (without :) */
    HttpHandler handler;
    void *user_data;
} Route;

/* Middleware entry */
typedef struct {
    char *prefix;           /* NULL for global middleware */
    HttpMiddleware middleware;
    void *user_data;
} Middleware;

/* Router structure */
struct HttpRouter {
    Route routes[MAX_ROUTES];
    size_t route_count;

    Middleware middlewares[MAX_MIDDLEWARES];
    size_t middleware_count;

    HttpHandler not_found_handler;
    void *not_found_data;

    HttpHandler method_not_allowed_handler;
    void *method_not_allowed_data;
};

/* Split path into segments */
static char **split_path(const char *path, size_t *count) {
    if (path == NULL || path[0] == '\0') {
        *count = 0;
        return NULL;
    }

    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }

    /* Count segments */
    size_t n = 0;
    if (path[0] != '\0') {
        n = 1;
        for (const char *p = path; *p; p++) {
            if (*p == '/') {
                n++;
            }
        }
    }

    char **segments = calloc(n + 1, sizeof(char *));
    if (segments == NULL) {
        *count = 0;
        return NULL;
    }

    /* Split */
    char *path_copy = str_dup(path);
    if (path_copy == NULL) {
        free(segments);
        *count = 0;
        return NULL;
    }

    size_t idx = 0;
    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);

    while (token != NULL && idx < n) {
        segments[idx] = str_dup(token);
        if (segments[idx] == NULL) {
            for (size_t i = 0; i < idx; i++) {
                free(segments[i]);
            }
            free(segments);
            free(path_copy);
            *count = 0;
            return NULL;
        }
        idx++;
        token = strtok_r(NULL, "/", &saveptr);
    }

    free(path_copy);
    *count = idx;
    return segments;
}

/* Free segments array */
static void free_segments(char **segments, size_t count) {
    if (segments == NULL) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(segments[i]);
    }
    free(segments);
}

/* Initialize a route */
static int init_route(Route *route, HttpMethod method, const char *pattern,
                      HttpHandler handler, void *user_data) {
    memset(route, 0, sizeof(Route));

    route->method = method;
    route->pattern = str_dup(pattern);
    if (route->pattern == NULL) {
        return -1;
    }

    route->segments = split_path(pattern, &route->segment_count);

    /* Allocate param tracking arrays */
    if (route->segment_count > 0) {
        route->is_param = calloc(route->segment_count, sizeof(bool));
        route->param_names = calloc(route->segment_count, sizeof(char *));

        if (route->is_param == NULL || route->param_names == NULL) {
            free(route->pattern);
            free_segments(route->segments, route->segment_count);
            free(route->is_param);
            free(route->param_names);
            return -1;
        }

        /* Identify parameters */
        for (size_t i = 0; i < route->segment_count; i++) {
            if (route->segments[i][0] == ':') {
                route->is_param[i] = true;
                route->param_names[i] = str_dup(route->segments[i] + 1);
            }
        }
    }

    route->handler = handler;
    route->user_data = user_data;

    return 0;
}

/* Free a route */
static void free_route(Route *route) {
    if (route == NULL) {
        return;
    }

    free(route->pattern);
    free_segments(route->segments, route->segment_count);

    if (route->param_names != NULL) {
        for (size_t i = 0; i < route->segment_count; i++) {
            free(route->param_names[i]);
        }
        free(route->param_names);
    }
    free(route->is_param);
}

/* Default 404 handler */
static void default_not_found(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    (void)user_data;
    http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Not Found");
}

/* Default 405 handler */
static void default_method_not_allowed(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    (void)user_data;
    http_response_error(resp, HTTP_STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed");
}

HttpRouter *http_router_create(void) {
    HttpRouter *router = calloc(1, sizeof(HttpRouter));
    if (router == NULL) {
        return NULL;
    }

    router->not_found_handler = default_not_found;
    router->method_not_allowed_handler = default_method_not_allowed;

    return router;
}

void http_router_free(HttpRouter *router) {
    if (router == NULL) {
        return;
    }

    for (size_t i = 0; i < router->route_count; i++) {
        free_route(&router->routes[i]);
    }

    for (size_t i = 0; i < router->middleware_count; i++) {
        free(router->middlewares[i].prefix);
    }

    free(router);
}

int http_router_add(HttpRouter *router, HttpMethod method, const char *pattern,
                    HttpHandler handler, void *user_data) {
    if (router == NULL || pattern == NULL || handler == NULL) {
        return -1;
    }

    if (router->route_count >= MAX_ROUTES) {
        LOG_ERROR("Maximum number of routes exceeded");
        return -1;
    }

    Route *route = &router->routes[router->route_count];
    if (init_route(route, method, pattern, handler, user_data) != 0) {
        return -1;
    }

    router->route_count++;
    LOG_DEBUG("Registered route: %s %s", http_method_to_string(method), pattern);

    return 0;
}

int http_router_use(HttpRouter *router, HttpMiddleware middleware, void *user_data) {
    return http_router_use_path(router, NULL, middleware, user_data);
}

int http_router_use_path(HttpRouter *router, const char *prefix,
                         HttpMiddleware middleware, void *user_data) {
    if (router == NULL || middleware == NULL) {
        return -1;
    }

    if (router->middleware_count >= MAX_MIDDLEWARES) {
        LOG_ERROR("Maximum number of middlewares exceeded");
        return -1;
    }

    Middleware *mw = &router->middlewares[router->middleware_count];
    mw->prefix = prefix ? str_dup(prefix) : NULL;
    mw->middleware = middleware;
    mw->user_data = user_data;

    router->middleware_count++;
    LOG_DEBUG("Registered middleware%s%s", prefix ? " for " : "", prefix ? prefix : "");

    return 0;
}

/* Check if request path matches middleware prefix */
static bool matches_prefix(const char *path, const char *prefix) {
    if (prefix == NULL) {
        return true;  /* Global middleware */
    }

    size_t prefix_len = strlen(prefix);
    if (strncmp(path, prefix, prefix_len) != 0) {
        return false;
    }

    /* Check for exact match or path continues with / */
    return path[prefix_len] == '\0' || path[prefix_len] == '/';
}

/* Check if route matches request and extract parameters */
static bool route_matches(Route *route, HttpRequest *req) {
    if (route->method != req->method) {
        return false;
    }

    /* Split request path */
    size_t req_count;
    char **req_segments = split_path(req->path, &req_count);

    /* Quick length check */
    if (req_count != route->segment_count) {
        free_segments(req_segments, req_count);
        return false;
    }

    /* Match each segment */
    for (size_t i = 0; i < route->segment_count; i++) {
        if (route->is_param[i]) {
            /* Parameter - matches anything */
            continue;
        }

        if (strcmp(route->segments[i], req_segments[i]) != 0) {
            free_segments(req_segments, req_count);
            return false;
        }
    }

    /* Match found - extract parameters */
    for (size_t i = 0; i < route->segment_count; i++) {
        if (route->is_param[i]) {
            http_request_add_path_param(req, route->param_names[i], req_segments[i]);
        }
    }

    free_segments(req_segments, req_count);
    return true;
}

/* Check if any route matches the path (regardless of method) */
static bool path_matches_any_route(HttpRouter *router, const char *path) {
    for (size_t i = 0; i < router->route_count; i++) {
        Route *route = &router->routes[i];

        size_t req_count;
        char **req_segments = split_path(path, &req_count);

        if (req_count != route->segment_count) {
            free_segments(req_segments, req_count);
            continue;
        }

        bool matches = true;
        for (size_t j = 0; j < route->segment_count; j++) {
            if (!route->is_param[j] && strcmp(route->segments[j], req_segments[j]) != 0) {
                matches = false;
                break;
            }
        }

        free_segments(req_segments, req_count);

        if (matches) {
            return true;
        }
    }
    return false;
}

bool http_router_handle(HttpRouter *router, HttpRequest *req, HttpResponse *resp) {
    if (router == NULL || req == NULL || resp == NULL) {
        return false;
    }

    /* Run middlewares */
    for (size_t i = 0; i < router->middleware_count; i++) {
        Middleware *mw = &router->middlewares[i];

        if (!matches_prefix(req->path, mw->prefix)) {
            continue;
        }

        if (!mw->middleware(req, resp, mw->user_data)) {
            /* Middleware stopped the chain */
            return true;
        }
    }

    /* Find matching route */
    for (size_t i = 0; i < router->route_count; i++) {
        if (route_matches(&router->routes[i], req)) {
            router->routes[i].handler(req, resp, router->routes[i].user_data);
            return true;
        }
    }

    /* No matching route found */
    /* Check if path exists but method doesn't match */
    if (path_matches_any_route(router, req->path)) {
        router->method_not_allowed_handler(req, resp, router->method_not_allowed_data);
    } else {
        router->not_found_handler(req, resp, router->not_found_data);
    }

    return true;
}

void http_router_set_not_found(HttpRouter *router, HttpHandler handler, void *user_data) {
    if (router == NULL || handler == NULL) {
        return;
    }
    router->not_found_handler = handler;
    router->not_found_data = user_data;
}

void http_router_set_method_not_allowed(HttpRouter *router, HttpHandler handler, void *user_data) {
    if (router == NULL || handler == NULL) {
        return;
    }
    router->method_not_allowed_handler = handler;
    router->method_not_allowed_data = user_data;
}

HttpRouteGroup *http_router_group(HttpRouter *router, const char *prefix) {
    if (router == NULL || prefix == NULL) {
        return NULL;
    }

    HttpRouteGroup *group = calloc(1, sizeof(HttpRouteGroup));
    if (group == NULL) {
        return NULL;
    }

    group->router = router;
    group->prefix = str_dup(prefix);

    if (group->prefix == NULL) {
        free(group);
        return NULL;
    }

    return group;
}

void http_route_group_free(HttpRouteGroup *group) {
    if (group == NULL) {
        return;
    }
    free(group->prefix);
    free(group);
}

int http_route_group_add(HttpRouteGroup *group, HttpMethod method, const char *pattern,
                         HttpHandler handler, void *user_data) {
    if (group == NULL || pattern == NULL || handler == NULL) {
        return -1;
    }

    char *full_pattern = str_concat(group->prefix, pattern);
    if (full_pattern == NULL) {
        return -1;
    }

    int result = http_router_add(group->router, method, full_pattern, handler, user_data);
    free(full_pattern);

    return result;
}

void http_router_print_routes(const HttpRouter *router) {
    if (router == NULL) {
        return;
    }

    LOG_INFO("Registered routes (%zu):", router->route_count);
    for (size_t i = 0; i < router->route_count; i++) {
        const Route *route = &router->routes[i];
        LOG_INFO("  %s %s", http_method_to_string(route->method), route->pattern);
    }
}
