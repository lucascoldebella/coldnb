#include "http/http_response.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HttpResponse *http_response_create(void) {
    HttpResponse *resp = calloc(1, sizeof(HttpResponse));
    if (resp == NULL) {
        return NULL;
    }
    resp->status = HTTP_STATUS_OK;
    return resp;
}

void http_response_free(HttpResponse *resp) {
    if (resp == NULL) {
        return;
    }

    for (size_t i = 0; i < resp->header_count; i++) {
        free(resp->headers[i].name);
        free(resp->headers[i].value);
    }

    free(resp->body);
    free(resp->content_type);
    free(resp);
}

void http_response_set_status(HttpResponse *resp, HttpStatus status) {
    if (resp != NULL) {
        resp->status = status;
    }
}

int http_response_add_header(HttpResponse *resp, const char *name, const char *value) {
    if (resp == NULL || name == NULL) {
        return -1;
    }

    if (resp->header_count >= HTTP_MAX_RESPONSE_HEADERS) {
        LOG_WARN("Too many response headers");
        return -1;
    }

    char *name_copy = str_dup(name);
    char *value_copy = value ? str_dup(value) : str_dup("");

    if (name_copy == NULL || value_copy == NULL) {
        free(name_copy);
        free(value_copy);
        return -1;
    }

    resp->headers[resp->header_count].name = name_copy;
    resp->headers[resp->header_count].value = value_copy;
    resp->header_count++;

    return 0;
}

void http_response_set_content_type(HttpResponse *resp, const char *content_type) {
    if (resp == NULL) {
        return;
    }
    free(resp->content_type);
    resp->content_type = content_type ? str_dup(content_type) : NULL;
}

int http_response_set_body(HttpResponse *resp, const char *body, size_t size) {
    if (resp == NULL) {
        return -1;
    }

    free(resp->body);
    resp->body = NULL;
    resp->body_size = 0;

    if (body == NULL || size == 0) {
        return 0;
    }

    resp->body = malloc(size + 1);
    if (resp->body == NULL) {
        return -1;
    }

    memcpy(resp->body, body, size);
    resp->body[size] = '\0';
    resp->body_size = size;

    return 0;
}

int http_response_set_body_string(HttpResponse *resp, const char *body) {
    if (body == NULL) {
        return http_response_set_body(resp, NULL, 0);
    }
    return http_response_set_body(resp, body, strlen(body));
}

int http_response_set_json(HttpResponse *resp, const char *json) {
    http_response_set_content_type(resp, "application/json; charset=utf-8");
    return http_response_set_body_string(resp, json);
}

const char *http_status_message(HttpStatus status) {
    switch (status) {
        case HTTP_STATUS_OK:                  return "OK";
        case HTTP_STATUS_CREATED:             return "Created";
        case HTTP_STATUS_NO_CONTENT:          return "No Content";
        case HTTP_STATUS_MOVED_PERMANENTLY:   return "Moved Permanently";
        case HTTP_STATUS_FOUND:               return "Found";
        case HTTP_STATUS_NOT_MODIFIED:        return "Not Modified";
        case HTTP_STATUS_BAD_REQUEST:         return "Bad Request";
        case HTTP_STATUS_UNAUTHORIZED:        return "Unauthorized";
        case HTTP_STATUS_FORBIDDEN:           return "Forbidden";
        case HTTP_STATUS_NOT_FOUND:           return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:  return "Method Not Allowed";
        case HTTP_STATUS_CONFLICT:            return "Conflict";
        case HTTP_STATUS_UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
        case HTTP_STATUS_TOO_MANY_REQUESTS:   return "Too Many Requests";
        case HTTP_STATUS_INTERNAL_ERROR:      return "Internal Server Error";
        case HTTP_STATUS_NOT_IMPLEMENTED:     return "Not Implemented";
        case HTTP_STATUS_BAD_GATEWAY:         return "Bad Gateway";
        case HTTP_STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default:                              return "Unknown";
    }
}

int http_response_json(HttpResponse *resp, HttpStatus status, const char *json) {
    http_response_set_status(resp, status);
    return http_response_set_json(resp, json);
}

int http_response_error(HttpResponse *resp, HttpStatus status, const char *message) {
    http_response_set_status(resp, status);

    char *json = str_printf("{\"error\":{\"status\":%d,\"message\":\"%s\"}}",
                            (int)status, message ? message : http_status_message(status));
    if (json == NULL) {
        return -1;
    }

    int result = http_response_set_json(resp, json);
    free(json);
    return result;
}

int http_response_success(HttpResponse *resp, const char *data_json) {
    http_response_set_status(resp, HTTP_STATUS_OK);

    if (data_json == NULL) {
        return http_response_set_json(resp, "{\"success\":true}");
    }

    char *json = str_printf("{\"success\":true,\"data\":%s}", data_json);
    if (json == NULL) {
        return -1;
    }

    int result = http_response_set_json(resp, json);
    free(json);
    return result;
}

int http_response_created(HttpResponse *resp, const char *data_json) {
    http_response_set_status(resp, HTTP_STATUS_CREATED);

    if (data_json == NULL) {
        return http_response_set_json(resp, "{\"success\":true}");
    }

    char *json = str_printf("{\"success\":true,\"data\":%s}", data_json);
    if (json == NULL) {
        return -1;
    }

    int result = http_response_set_json(resp, json);
    free(json);
    return result;
}

void http_response_no_content(HttpResponse *resp) {
    if (resp == NULL) {
        return;
    }
    http_response_set_status(resp, HTTP_STATUS_NO_CONTENT);
    http_response_set_body(resp, NULL, 0);
}

int http_response_redirect(HttpResponse *resp, const char *location, bool permanent) {
    if (resp == NULL || location == NULL) {
        return -1;
    }

    http_response_set_status(resp, permanent ?
                             HTTP_STATUS_MOVED_PERMANENTLY : HTTP_STATUS_FOUND);
    return http_response_add_header(resp, "Location", location);
}

void http_response_set_cors(HttpResponse *resp, const char *origin,
                            const char *methods, const char *headers) {
    if (resp == NULL) {
        return;
    }

    if (origin != NULL) {
        http_response_add_header(resp, "Access-Control-Allow-Origin", origin);
    }
    if (methods != NULL) {
        http_response_add_header(resp, "Access-Control-Allow-Methods", methods);
    }
    if (headers != NULL) {
        http_response_add_header(resp, "Access-Control-Allow-Headers", headers);
    }
    http_response_add_header(resp, "Access-Control-Allow-Credentials", "true");
}

void http_response_set_cache(HttpResponse *resp, int max_age_seconds) {
    if (resp == NULL) {
        return;
    }

    char cache_control[64];
    snprintf(cache_control, sizeof(cache_control), "public, max-age=%d", max_age_seconds);
    http_response_add_header(resp, "Cache-Control", cache_control);
}

void http_response_no_cache(HttpResponse *resp) {
    if (resp == NULL) {
        return;
    }
    http_response_add_header(resp, "Cache-Control", "no-store, no-cache, must-revalidate");
    http_response_add_header(resp, "Pragma", "no-cache");
}
