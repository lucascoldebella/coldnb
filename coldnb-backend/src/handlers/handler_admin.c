#include "handlers/handler_admin.h"
#include "services/svc_admin_auth.h"
#include "auth/auth_middleware.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_admin_register(HttpRouter *router, DbPool *pool) {
    /* Login is public (no auth required) */
    ROUTE_POST(router, "/api/admin/login", handler_admin_login, pool);

    /* Logout and me require admin auth (middleware applied in main.c) */
    ROUTE_POST(router, "/api/admin/logout", handler_admin_logout, pool);
    ROUTE_GET(router, "/api/admin/me", handler_admin_me, pool);
}

void handler_admin_login(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *username = json_get_string(body, "username", NULL);
    const char *password = json_get_string(body, "password", NULL);

    if (str_is_empty(username) || str_is_empty(password)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                           "Username and password are required");
        return;
    }

    /* Get client info */
    const char *ip_address = req->client_ip;
    const char *user_agent = http_request_get_header(req, "User-Agent");

    /* Attempt login */
    AdminSession *session = admin_auth_login(pool, username, password,
                                             ip_address, user_agent);
    cJSON_Delete(body);

    if (session == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED,
                           "Invalid username or password");
        return;
    }

    /* Get admin user info */
    AdminUser *user = admin_auth_get_user(pool, session->admin_id);

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "token", session->token);
    cJSON_AddNumberToObject(data, "expires_at", (double)session->expires_at);

    if (user != NULL) {
        cJSON *user_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(user_obj, "id", user->id);
        cJSON_AddStringToObject(user_obj, "username", user->username);
        json_add_string_if(user_obj, "email", user->email);
        json_add_string_if(user_obj, "full_name", user->full_name);
        cJSON_AddStringToObject(user_obj, "role", user->role);
        cJSON_AddItemToObject(data, "user", user_obj);
        admin_user_free(user);
    }

    admin_session_free(session);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Admin login: %s", username);
}

void handler_admin_logout(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "No token provided");
        return;
    }

    int result = admin_auth_logout(pool, token);
    if (result != 0) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Logout failed");
        return;
    }

    http_response_success(resp, "{\"message\":\"Logged out successfully\"}");
    LOG_INFO("Admin logout");
}

void handler_admin_me(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "No token provided");
        return;
    }

    AdminUser *user = admin_auth_validate_token(pool, token);
    if (user == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Invalid or expired token");
        return;
    }

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", user->id);
    cJSON_AddStringToObject(data, "username", user->username);
    json_add_string_if(data, "email", user->email);
    json_add_string_if(data, "full_name", user->full_name);
    cJSON_AddStringToObject(data, "role", user->role);
    cJSON_AddBoolToObject(data, "is_active", user->is_active);

    admin_user_free(user);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
