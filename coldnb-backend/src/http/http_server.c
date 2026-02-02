#include "http/http_server.h"
#include "http/http_request.h"
#include "http/http_response.h"
#include "util/string_util.h"
#include "log/log.h"

#include <microhttpd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/* Connection context for tracking request data */
typedef struct {
    HttpRequest *request;
    HttpServer *server;
} ConnectionContext;

/* Server structure */
struct HttpServer {
    HttpServerConfig config;
    struct MHD_Daemon *daemon;
    HttpRouter *router;
    volatile bool running;
    pthread_mutex_t lock;
};

/* Collect headers from libmicrohttpd */
static enum MHD_Result collect_headers(void *cls, enum MHD_ValueKind kind,
                                       const char *key, const char *value) {
    (void)kind;
    HttpRequest *req = (HttpRequest *)cls;

    if (req->header_count < HTTP_MAX_HEADERS) {
        req->headers[req->header_count].name = key;
        req->headers[req->header_count].value = value;
        req->header_count++;
    }

    return MHD_YES;
}

/* Build MHD response from HttpResponse */
static struct MHD_Response *build_mhd_response(HttpResponse *resp) {
    struct MHD_Response *mhd_resp;

    if (resp->body != NULL && resp->body_size > 0) {
        mhd_resp = MHD_create_response_from_buffer(
            resp->body_size,
            resp->body,
            MHD_RESPMEM_MUST_COPY
        );
    } else {
        mhd_resp = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
    }

    if (mhd_resp == NULL) {
        return NULL;
    }

    /* Add Content-Type */
    if (resp->content_type != NULL) {
        MHD_add_response_header(mhd_resp, "Content-Type", resp->content_type);
    }

    /* Add custom headers */
    for (size_t i = 0; i < resp->header_count; i++) {
        MHD_add_response_header(mhd_resp, resp->headers[i].name, resp->headers[i].value);
    }

    return mhd_resp;
}

/* Handle CORS preflight */
static void handle_cors(HttpServer *server, HttpRequest *req, HttpResponse *resp) {
    if (server->config.cors_origins == NULL) {
        return;
    }

    const char *origin = http_request_get_header(req, "Origin");
    if (origin == NULL) {
        return;
    }

    /* Check if origin is allowed */
    bool allowed = false;
    if (strcmp(server->config.cors_origins, "*") == 0) {
        allowed = true;
    } else {
        /* Check against comma-separated list */
        char *origins_copy = str_dup(server->config.cors_origins);
        if (origins_copy != NULL) {
            char *saveptr;
            char *token = strtok_r(origins_copy, ",", &saveptr);
            while (token != NULL) {
                char *trimmed = str_trim(token);
                if (strcmp(trimmed, origin) == 0) {
                    allowed = true;
                    break;
                }
                token = strtok_r(NULL, ",", &saveptr);
            }
            free(origins_copy);
        }
    }

    if (!allowed) {
        return;
    }

    http_response_set_cors(resp,
                           strcmp(server->config.cors_origins, "*") == 0 ? "*" : origin,
                           server->config.cors_methods,
                           server->config.cors_headers);
}

/* Main request handler callback */
static enum MHD_Result request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method,
                                       const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls) {
    (void)version;
    HttpServer *server = (HttpServer *)cls;
    ConnectionContext *ctx = *con_cls;

    /* First call - create context */
    if (ctx == NULL) {
        ctx = calloc(1, sizeof(ConnectionContext));
        if (ctx == NULL) {
            return MHD_NO;
        }

        ctx->server = server;
        ctx->request = http_request_create();
        if (ctx->request == NULL) {
            free(ctx);
            return MHD_NO;
        }

        /* Parse URL */
        ctx->request->raw_url = str_dup(url);
        ctx->request->method = http_method_from_string(method);
        ctx->request->connection = connection;

        /* Split path and query string */
        char *query = strchr(url, '?');
        if (query != NULL) {
            ctx->request->path = str_ndup(url, (size_t)(query - url));
            ctx->request->query_string = str_dup(query + 1);
            http_request_parse_query_string(ctx->request);
        } else {
            ctx->request->path = str_dup(url);
        }

        /* Collect headers */
        MHD_get_connection_values(connection, MHD_HEADER_KIND,
                                  collect_headers, ctx->request);

        /* Get client IP */
        const union MHD_ConnectionInfo *info =
            MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
        if (info != NULL && info->client_addr != NULL) {
            /* We'll just store the pointer - MHD owns this */
            ctx->request->client_ip = "127.0.0.1";  /* Default to localhost */
        }

        *con_cls = ctx;
        return MHD_YES;
    }

    /* Subsequent calls - handle upload data */
    if (*upload_data_size > 0) {
        /* Check size limit */
        if (ctx->request->body_size + *upload_data_size > server->config.max_request_size) {
            LOG_WARN("Request body too large");
            *upload_data_size = 0;
            return MHD_NO;
        }

        if (http_request_append_body(ctx->request, upload_data, *upload_data_size) != 0) {
            return MHD_NO;
        }
        *upload_data_size = 0;
        return MHD_YES;
    }

    /* All data received - process request */
    HttpResponse *resp = http_response_create();
    if (resp == NULL) {
        return MHD_NO;
    }

    /* Handle CORS */
    handle_cors(server, ctx->request, resp);

    /* Handle OPTIONS preflight */
    if (ctx->request->method == HTTP_METHOD_OPTIONS) {
        http_response_no_content(resp);
    } else {
        /* Route the request */
        http_router_handle(server->router, ctx->request, resp);
    }

    /* Build and send response */
    struct MHD_Response *mhd_resp = build_mhd_response(resp);
    if (mhd_resp == NULL) {
        http_response_free(resp);
        return MHD_NO;
    }

    enum MHD_Result ret = MHD_queue_response(connection, resp->status, mhd_resp);
    MHD_destroy_response(mhd_resp);

    /* Log request */
    LOG_INFO("%s %s %d", http_method_to_string(ctx->request->method),
             ctx->request->path, resp->status);

    http_response_free(resp);

    return ret;
}

/* Cleanup callback when connection closes */
static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;

    ConnectionContext *ctx = *con_cls;
    if (ctx != NULL) {
        http_request_free(ctx->request);
        free(ctx);
    }
    *con_cls = NULL;
}

void http_server_config_defaults(HttpServerConfig *config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(HttpServerConfig));
    config->port = HTTP_SERVER_DEFAULT_PORT;
    config->thread_pool_size = HTTP_SERVER_DEFAULT_THREADS;
    config->max_connections = HTTP_SERVER_DEFAULT_MAX_CONNECTIONS;
    config->request_timeout = HTTP_SERVER_DEFAULT_TIMEOUT;
    config->max_request_size = HTTP_SERVER_DEFAULT_MAX_REQUEST_SIZE;
    config->use_thread_per_connection = false;
}

HttpServer *http_server_create(const HttpServerConfig *config) {
    HttpServer *server = calloc(1, sizeof(HttpServer));
    if (server == NULL) {
        return NULL;
    }

    if (config != NULL) {
        server->config = *config;
    } else {
        http_server_config_defaults(&server->config);
    }

    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        free(server);
        return NULL;
    }

    server->router = http_router_create();
    if (server->router == NULL) {
        pthread_mutex_destroy(&server->lock);
        free(server);
        return NULL;
    }

    return server;
}

void http_server_free(HttpServer *server) {
    if (server == NULL) {
        return;
    }

    http_server_stop(server);
    http_router_free(server->router);
    pthread_mutex_destroy(&server->lock);
    free(server);
}

HttpRouter *http_server_get_router(HttpServer *server) {
    if (server == NULL) {
        return NULL;
    }
    return server->router;
}

int http_server_start(HttpServer *server) {
    if (server == NULL) {
        return -1;
    }

    /* Determine threading mode */
    unsigned int flags = MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD;

    if (server->config.use_thread_per_connection) {
        flags |= MHD_USE_THREAD_PER_CONNECTION;
    }

    /* Start daemon */
    server->daemon = MHD_start_daemon(
        flags,
        (uint16_t)server->config.port,
        NULL,                           /* Accept policy callback */
        NULL,                           /* Accept policy callback data */
        request_handler,                /* Request handler */
        server,                         /* Request handler data */
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)server->config.request_timeout,
        MHD_OPTION_THREAD_POOL_SIZE, (unsigned int)server->config.thread_pool_size,
        MHD_OPTION_CONNECTION_LIMIT, (unsigned int)server->config.max_connections,
        MHD_OPTION_END
    );

    if (server->daemon == NULL) {
        LOG_ERROR("Failed to start HTTP server on port %d", server->config.port);
        return -1;
    }

    server->running = true;
    LOG_INFO("HTTP server started on port %d", server->config.port);

    /* Block until shutdown requested */
    while (server->running) {
        sleep(1);
    }

    /* Stop daemon */
    MHD_stop_daemon(server->daemon);
    server->daemon = NULL;

    LOG_INFO("HTTP server stopped");
    return 0;
}

void http_server_stop(HttpServer *server) {
    if (server == NULL) {
        return;
    }
    server->running = false;
}

bool http_server_is_running(HttpServer *server) {
    if (server == NULL) {
        return false;
    }
    return server->running;
}

int http_server_get_port(HttpServer *server) {
    if (server == NULL) {
        return -1;
    }
    return server->config.port;
}
