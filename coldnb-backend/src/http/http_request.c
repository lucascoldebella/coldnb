#include "http/http_request.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define INITIAL_BODY_CAPACITY 4096

HttpRequest *http_request_create(void) {
    HttpRequest *req = calloc(1, sizeof(HttpRequest));
    if (req == NULL) {
        return NULL;
    }
    req->method = HTTP_METHOD_UNKNOWN;
    return req;
}

void http_request_free(HttpRequest *req) {
    if (req == NULL) {
        return;
    }

    /* Free user_data (e.g. AuthContext) if cleanup function is set */
    if (req->user_data != NULL && req->user_data_free != NULL) {
        req->user_data_free(req->user_data);
        req->user_data = NULL;
    }

    free(req->path);
    free(req->query_string);
    free(req->raw_url);
    free(req->body);

    /* Free query params */
    for (size_t i = 0; i < req->query_param_count; i++) {
        free(req->query_params[i].name);
        free(req->query_params[i].value);
    }

    /* Free path params */
    for (size_t i = 0; i < req->path_param_count; i++) {
        free(req->path_params[i].name);
        free(req->path_params[i].value);
    }

    free(req);
}

HttpMethod http_method_from_string(const char *str) {
    if (str == NULL) {
        return HTTP_METHOD_UNKNOWN;
    }

    if (strcasecmp(str, "GET") == 0) {
        return HTTP_METHOD_GET;
    }
    if (strcasecmp(str, "POST") == 0) {
        return HTTP_METHOD_POST;
    }
    if (strcasecmp(str, "PUT") == 0) {
        return HTTP_METHOD_PUT;
    }
    if (strcasecmp(str, "DELETE") == 0) {
        return HTTP_METHOD_DELETE;
    }
    if (strcasecmp(str, "PATCH") == 0) {
        return HTTP_METHOD_PATCH;
    }
    if (strcasecmp(str, "OPTIONS") == 0) {
        return HTTP_METHOD_OPTIONS;
    }
    if (strcasecmp(str, "HEAD") == 0) {
        return HTTP_METHOD_HEAD;
    }

    return HTTP_METHOD_UNKNOWN;
}

const char *http_method_to_string(HttpMethod method) {
    switch (method) {
        case HTTP_METHOD_GET:     return "GET";
        case HTTP_METHOD_POST:    return "POST";
        case HTTP_METHOD_PUT:     return "PUT";
        case HTTP_METHOD_DELETE:  return "DELETE";
        case HTTP_METHOD_PATCH:   return "PATCH";
        case HTTP_METHOD_OPTIONS: return "OPTIONS";
        case HTTP_METHOD_HEAD:    return "HEAD";
        default:                  return "UNKNOWN";
    }
}

const char *http_request_get_header(const HttpRequest *req, const char *name) {
    if (req == NULL || name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char *http_request_get_query_param(const HttpRequest *req, const char *name) {
    if (req == NULL || name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < req->query_param_count; i++) {
        if (strcmp(req->query_params[i].name, name) == 0) {
            return req->query_params[i].value;
        }
    }
    return NULL;
}

const char *http_request_get_path_param(const HttpRequest *req, const char *name) {
    if (req == NULL || name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < req->path_param_count; i++) {
        if (strcmp(req->path_params[i].name, name) == 0) {
            return req->path_params[i].value;
        }
    }
    return NULL;
}

int http_request_add_path_param(HttpRequest *req, const char *name, const char *value) {
    if (req == NULL || name == NULL) {
        return -1;
    }

    if (req->path_param_count >= HTTP_MAX_PARAMS) {
        LOG_WARN("Too many path parameters");
        return -1;
    }

    char *name_copy = str_dup(name);
    char *value_copy = value ? str_url_decode(value) : NULL;

    if (name_copy == NULL || (value != NULL && value_copy == NULL)) {
        free(name_copy);
        free(value_copy);
        return -1;
    }

    req->path_params[req->path_param_count].name = name_copy;
    req->path_params[req->path_param_count].value = value_copy;
    req->path_param_count++;

    return 0;
}

int http_request_parse_query_string(HttpRequest *req) {
    if (req == NULL || req->query_string == NULL || req->query_string[0] == '\0') {
        return 0;
    }

    /* Make a copy to tokenize */
    char *query_copy = str_dup(req->query_string);
    if (query_copy == NULL) {
        return -1;
    }

    char *saveptr;
    char *pair = strtok_r(query_copy, "&", &saveptr);

    while (pair != NULL && req->query_param_count < HTTP_MAX_PARAMS) {
        char *equals = strchr(pair, '=');

        char *name;
        char *value;

        if (equals != NULL) {
            *equals = '\0';
            name = str_url_decode(pair);
            value = str_url_decode(equals + 1);
        } else {
            name = str_url_decode(pair);
            value = str_dup("");
        }

        if (name == NULL) {
            free(value);
            pair = strtok_r(NULL, "&", &saveptr);
            continue;
        }

        req->query_params[req->query_param_count].name = name;
        req->query_params[req->query_param_count].value = value;
        req->query_param_count++;

        pair = strtok_r(NULL, "&", &saveptr);
    }

    free(query_copy);
    return 0;
}

int http_request_append_body(HttpRequest *req, const char *data, size_t size) {
    if (req == NULL || data == NULL || size == 0) {
        return 0;
    }

    /* Ensure capacity */
    size_t new_size = req->body_size + size;
    if (new_size > req->body_capacity) {
        size_t new_capacity = req->body_capacity;
        if (new_capacity == 0) {
            new_capacity = INITIAL_BODY_CAPACITY;
        }
        while (new_capacity < new_size) {
            new_capacity *= 2;
        }

        char *new_body = realloc(req->body, new_capacity + 1);
        if (new_body == NULL) {
            LOG_ERROR("Failed to allocate request body");
            return -1;
        }
        req->body = new_body;
        req->body_capacity = new_capacity;
    }

    memcpy(req->body + req->body_size, data, size);
    req->body_size = new_size;
    req->body[req->body_size] = '\0';

    return 0;
}

const char *http_request_get_content_type(const HttpRequest *req) {
    return http_request_get_header(req, "Content-Type");
}

const char *http_request_get_authorization(const HttpRequest *req) {
    return http_request_get_header(req, "Authorization");
}

bool http_request_is_json(const HttpRequest *req) {
    const char *ct = http_request_get_content_type(req);
    if (ct == NULL) {
        return false;
    }
    return strstr(ct, "application/json") != NULL;
}

const char *http_request_get_bearer_token(const HttpRequest *req) {
    const char *auth = http_request_get_authorization(req);
    if (auth == NULL) {
        return NULL;
    }

    /* Check for "Bearer " prefix (case-insensitive) */
    if (strncasecmp(auth, "Bearer ", 7) != 0) {
        return NULL;
    }

    const char *token = auth + 7;

    /* Skip whitespace */
    while (*token == ' ') {
        token++;
    }

    if (*token == '\0') {
        return NULL;
    }

    return token;
}
