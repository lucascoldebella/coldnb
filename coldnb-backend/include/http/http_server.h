#ifndef COLDNB_HTTP_SERVER_H
#define COLDNB_HTTP_SERVER_H

#include "http/http_router.h"
#include <stdbool.h>
#include <stddef.h>

/* Forward declaration */
typedef struct HttpServer HttpServer;

/* Server configuration */
typedef struct {
    int port;
    int thread_pool_size;
    size_t max_connections;
    size_t request_timeout;       /* Seconds */
    size_t max_request_size;      /* Bytes */
    bool use_thread_per_connection;

    /* CORS settings */
    const char *cors_origins;     /* Comma-separated or "*" */
    const char *cors_methods;
    const char *cors_headers;
} HttpServerConfig;

/* Create a new HTTP server */
HttpServer *http_server_create(const HttpServerConfig *config);

/* Free HTTP server */
void http_server_free(HttpServer *server);

/* Get the router for registering routes */
HttpRouter *http_server_get_router(HttpServer *server);

/* Start the server (blocking)
 * Returns 0 on clean shutdown, -1 on error */
int http_server_start(HttpServer *server);

/* Request server shutdown (can be called from signal handler) */
void http_server_stop(HttpServer *server);

/* Check if server is running */
bool http_server_is_running(HttpServer *server);

/* Get server port */
int http_server_get_port(HttpServer *server);

/* Default configuration values */
#define HTTP_SERVER_DEFAULT_PORT 8080
#define HTTP_SERVER_DEFAULT_THREADS 4
#define HTTP_SERVER_DEFAULT_MAX_CONNECTIONS 1000
#define HTTP_SERVER_DEFAULT_TIMEOUT 30
#define HTTP_SERVER_DEFAULT_MAX_REQUEST_SIZE (10 * 1024 * 1024)  /* 10 MB */

/* Initialize config with defaults */
void http_server_config_defaults(HttpServerConfig *config);

#endif /* COLDNB_HTTP_SERVER_H */
