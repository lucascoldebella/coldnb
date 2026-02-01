#ifndef COLDNB_HTTP_RESPONSE_H
#define COLDNB_HTTP_RESPONSE_H

#include <stddef.h>
#include <stdbool.h>

/* Common HTTP status codes */
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,
    HTTP_STATUS_TOO_MANY_REQUESTS = 429,
    HTTP_STATUS_INTERNAL_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503
} HttpStatus;

/* Header entry */
typedef struct {
    char *name;
    char *value;
} HttpResponseHeader;

/* Maximum number of response headers */
#define HTTP_MAX_RESPONSE_HEADERS 32

/* HTTP response structure */
typedef struct HttpResponse {
    HttpStatus status;
    HttpResponseHeader headers[HTTP_MAX_RESPONSE_HEADERS];
    size_t header_count;
    char *body;
    size_t body_size;
    char *content_type;
    bool headers_sent;
} HttpResponse;

/* Create a new HTTP response */
HttpResponse *http_response_create(void);

/* Free HTTP response */
void http_response_free(HttpResponse *resp);

/* Set response status */
void http_response_set_status(HttpResponse *resp, HttpStatus status);

/* Add a header */
int http_response_add_header(HttpResponse *resp, const char *name, const char *value);

/* Set Content-Type header */
void http_response_set_content_type(HttpResponse *resp, const char *content_type);

/* Set response body (copies data) */
int http_response_set_body(HttpResponse *resp, const char *body, size_t size);

/* Set response body from string (null-terminated) */
int http_response_set_body_string(HttpResponse *resp, const char *body);

/* Set JSON body with appropriate Content-Type */
int http_response_set_json(HttpResponse *resp, const char *json);

/* Get HTTP status message */
const char *http_status_message(HttpStatus status);

/* Convenience functions for common responses */

/* Send JSON response with status */
int http_response_json(HttpResponse *resp, HttpStatus status, const char *json);

/* Send error response as JSON */
int http_response_error(HttpResponse *resp, HttpStatus status, const char *message);

/* Send success response with data */
int http_response_success(HttpResponse *resp, const char *data_json);

/* Send created response with data */
int http_response_created(HttpResponse *resp, const char *data_json);

/* Send no content response */
void http_response_no_content(HttpResponse *resp);

/* Send redirect response */
int http_response_redirect(HttpResponse *resp, const char *location, bool permanent);

/* Set CORS headers */
void http_response_set_cors(HttpResponse *resp, const char *origin,
                            const char *methods, const char *headers);

/* Set cache control headers */
void http_response_set_cache(HttpResponse *resp, int max_age_seconds);

/* Disable caching */
void http_response_no_cache(HttpResponse *resp);

#endif /* COLDNB_HTTP_RESPONSE_H */
