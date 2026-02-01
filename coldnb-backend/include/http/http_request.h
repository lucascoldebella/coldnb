#ifndef COLDNB_HTTP_REQUEST_H
#define COLDNB_HTTP_REQUEST_H

#include <stddef.h>
#include <stdbool.h>

/* HTTP methods */
typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_UNKNOWN
} HttpMethod;

/* Maximum sizes */
#define HTTP_MAX_HEADERS 64
#define HTTP_MAX_PARAMS 32
#define HTTP_MAX_PATH 2048
#define HTTP_MAX_HEADER_VALUE 4096

/* Header entry */
typedef struct {
    const char *name;
    const char *value;
} HttpHeader;

/* Query/path parameter entry */
typedef struct {
    char *name;
    char *value;
} HttpParam;

/* HTTP request structure */
typedef struct HttpRequest {
    HttpMethod method;
    char *path;                           /* URL path without query string */
    char *query_string;                   /* Raw query string */
    char *raw_url;                        /* Full URL */

    /* Headers (borrowed from microhttpd) */
    HttpHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;

    /* Query parameters (owned) */
    HttpParam query_params[HTTP_MAX_PARAMS];
    size_t query_param_count;

    /* Path parameters (set by router) */
    HttpParam path_params[HTTP_MAX_PARAMS];
    size_t path_param_count;

    /* Request body */
    char *body;
    size_t body_size;
    size_t body_capacity;

    /* Client info */
    const char *client_ip;

    /* Internal state */
    void *connection;                     /* MHD_Connection pointer */
    void *user_data;                      /* For handler use */
} HttpRequest;

/* Create a new HTTP request */
HttpRequest *http_request_create(void);

/* Free HTTP request */
void http_request_free(HttpRequest *req);

/* Parse HTTP method from string */
HttpMethod http_method_from_string(const char *str);

/* Get HTTP method name */
const char *http_method_to_string(HttpMethod method);

/* Get header value by name (case-insensitive) */
const char *http_request_get_header(const HttpRequest *req, const char *name);

/* Get query parameter by name */
const char *http_request_get_query_param(const HttpRequest *req, const char *name);

/* Get path parameter by name */
const char *http_request_get_path_param(const HttpRequest *req, const char *name);

/* Add a path parameter (used by router) */
int http_request_add_path_param(HttpRequest *req, const char *name, const char *value);

/* Parse query string and populate query_params
 * Returns 0 on success, -1 on error */
int http_request_parse_query_string(HttpRequest *req);

/* Append data to request body
 * Returns 0 on success, -1 on error */
int http_request_append_body(HttpRequest *req, const char *data, size_t size);

/* Get Content-Type header */
const char *http_request_get_content_type(const HttpRequest *req);

/* Get Authorization header */
const char *http_request_get_authorization(const HttpRequest *req);

/* Check if request has JSON content type */
bool http_request_is_json(const HttpRequest *req);

/* Get bearer token from Authorization header
 * Returns NULL if not present or not Bearer token */
const char *http_request_get_bearer_token(const HttpRequest *req);

#endif /* COLDNB_HTTP_REQUEST_H */
