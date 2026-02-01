#include "handlers/handler_user.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_user_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/user/profile", handler_user_profile_get, pool);
    ROUTE_PUT(router, "/api/user/profile", handler_user_profile_update, pool);
}

void handler_user_profile_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT id, email, full_name, phone, avatar_url, email_verified, created_at "
        "FROM users WHERE id = $1 OR supabase_id = $1";
    const char *params[] = { user_id };

    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", db_row_get_string(&row, "id"));
    cJSON_AddStringToObject(data, "email", db_row_get_string(&row, "email"));
    json_add_string_if(data, "full_name", db_row_get_string(&row, "full_name"));
    json_add_string_if(data, "phone", db_row_get_string(&row, "phone"));
    json_add_string_if(data, "avatar_url", db_row_get_string(&row, "avatar_url"));
    cJSON_AddBoolToObject(data, "email_verified", db_row_get_bool(&row, "email_verified"));
    cJSON_AddStringToObject(data, "created_at", db_row_get_string(&row, "created_at"));

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_user_profile_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    /* Extract updateable fields */
    const char *full_name = json_get_string(body, "full_name", NULL);
    const char *phone = json_get_string(body, "phone", NULL);
    const char *avatar_url = json_get_string(body, "avatar_url", NULL);

    /* Validate phone format if provided */
    if (phone != NULL && strlen(phone) > 50) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Phone number too long");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build dynamic update query */
    const char *update_query =
        "UPDATE users SET "
        "full_name = COALESCE($2, full_name), "
        "phone = COALESCE($3, phone), "
        "avatar_url = COALESCE($4, avatar_url), "
        "updated_at = NOW() "
        "WHERE id = $1 OR supabase_id = $1 "
        "RETURNING id, email, full_name, phone, avatar_url, email_verified";

    const char *params[] = { user_id, full_name, phone, avatar_url };
    PGresult *result = db_exec_params(conn, update_query, 4, params);

    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", db_row_get_string(&row, "id"));
    cJSON_AddStringToObject(data, "email", db_row_get_string(&row, "email"));
    json_add_string_if(data, "full_name", db_row_get_string(&row, "full_name"));
    json_add_string_if(data, "phone", db_row_get_string(&row, "phone"));
    json_add_string_if(data, "avatar_url", db_row_get_string(&row, "avatar_url"));
    cJSON_AddBoolToObject(data, "email_verified", db_row_get_bool(&row, "email_verified"));

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Profile updated for user: %s", user_id);
}
