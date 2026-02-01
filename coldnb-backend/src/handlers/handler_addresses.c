#include "handlers/handler_addresses.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_addresses_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/addresses", handler_addresses_list, pool);
    ROUTE_GET(router, "/api/addresses/:id", handler_addresses_get, pool);
    ROUTE_POST(router, "/api/addresses", handler_addresses_create, pool);
    ROUTE_PUT(router, "/api/addresses/:id", handler_addresses_update, pool);
    ROUTE_DELETE(router, "/api/addresses/:id", handler_addresses_delete, pool);
    ROUTE_PUT(router, "/api/addresses/:id/default", handler_addresses_set_default, pool);
}

void handler_addresses_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    /* Get user's internal ID first (handle supabase_id) */
    const char *user_query = "SELECT id FROM users WHERE id = $1 OR supabase_id = $1";
    const char *user_params[] = { user_id };
    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    const char *internal_user_id = db_result_value(user_result);
    char *user_id_copy = str_dup(internal_user_id);
    PQclear(user_result);

    const char *query =
        "SELECT id, label, recipient_name, phone, street_address, street_address_2, "
        "city, state, postal_code, country, is_default, created_at "
        "FROM user_addresses WHERE user_id = $1 "
        "ORDER BY is_default DESC, created_at DESC";
    const char *params[] = { user_id_copy };

    PGresult *result = db_exec_params(conn, query, 1, params);
    free(user_id_copy);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *addresses = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "addresses", addresses);
    cJSON_AddNumberToObject(data, "count", cJSON_GetArraySize(addresses));

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_addresses_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *address_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (address_id == NULL || !uuid_validate(address_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid address ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT a.id, a.label, a.recipient_name, a.phone, a.street_address, "
        "a.street_address_2, a.city, a.state, a.postal_code, a.country, "
        "a.is_default, a.created_at "
        "FROM user_addresses a "
        "JOIN users u ON a.user_id = u.id "
        "WHERE a.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *params[] = { address_id, user_id };

    PGresult *result = db_exec_params(conn, query, 2, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Address not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_addresses_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    /* Extract and validate fields */
    const char *recipient_name = json_get_string(body, "recipient_name", NULL);
    const char *street_address = json_get_string(body, "street_address", NULL);
    const char *city = json_get_string(body, "city", NULL);
    const char *state = json_get_string(body, "state", NULL);
    const char *postal_code = json_get_string(body, "postal_code", NULL);

    if (str_is_empty(recipient_name) || str_is_empty(street_address) ||
        str_is_empty(city) || str_is_empty(state) || str_is_empty(postal_code)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                           "Required fields: recipient_name, street_address, city, state, postal_code");
        return;
    }

    const char *label = json_get_string(body, "label", NULL);
    const char *phone = json_get_string(body, "phone", NULL);
    const char *street_address_2 = json_get_string(body, "street_address_2", NULL);
    const char *country = json_get_string(body, "country", "Brazil");
    bool is_default = json_get_bool(body, "is_default", false);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get user's internal ID */
    const char *user_query = "SELECT id FROM users WHERE id = $1 OR supabase_id = $1";
    const char *user_params[] = { user_id };
    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    const char *internal_user_id = db_result_value(user_result);
    char *user_id_copy = str_dup(internal_user_id);
    PQclear(user_result);

    /* Begin transaction */
    if (!db_begin(conn)) {
        free(user_id_copy);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* If this is the default address, unset other defaults */
    if (is_default) {
        const char *unset_query =
            "UPDATE user_addresses SET is_default = false WHERE user_id = $1";
        const char *unset_params[] = { user_id_copy };
        PGresult *unset_result = db_exec_params(conn, unset_query, 1, unset_params);
        PQclear(unset_result);
    }

    /* Insert new address */
    const char *insert_query =
        "INSERT INTO user_addresses "
        "(user_id, label, recipient_name, phone, street_address, street_address_2, "
        "city, state, postal_code, country, is_default) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) "
        "RETURNING id, label, recipient_name, phone, street_address, street_address_2, "
        "city, state, postal_code, country, is_default, created_at";

    const char *is_default_str = is_default ? "true" : "false";
    const char *insert_params[] = {
        user_id_copy, label, recipient_name, phone, street_address,
        street_address_2, city, state, postal_code, country, is_default_str
    };

    PGresult *result = db_exec_params(conn, insert_query, 11, insert_params);
    free(user_id_copy);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_rollback(conn);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create address");
        return;
    }

    db_commit(conn);

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Address created for user: %s", user_id);
}

void handler_addresses_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *address_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (address_id == NULL || !uuid_validate(address_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid address ID");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Update address (only if owned by user) */
    const char *update_query =
        "UPDATE user_addresses a SET "
        "label = COALESCE($3, a.label), "
        "recipient_name = COALESCE($4, a.recipient_name), "
        "phone = COALESCE($5, a.phone), "
        "street_address = COALESCE($6, a.street_address), "
        "street_address_2 = COALESCE($7, a.street_address_2), "
        "city = COALESCE($8, a.city), "
        "state = COALESCE($9, a.state), "
        "postal_code = COALESCE($10, a.postal_code), "
        "country = COALESCE($11, a.country), "
        "updated_at = NOW() "
        "FROM users u "
        "WHERE a.id = $1 AND a.user_id = u.id AND (u.id = $2 OR u.supabase_id = $2) "
        "RETURNING a.id, a.label, a.recipient_name, a.phone, a.street_address, "
        "a.street_address_2, a.city, a.state, a.postal_code, a.country, "
        "a.is_default, a.created_at";

    const char *params[] = {
        address_id,
        user_id,
        json_get_string(body, "label", NULL),
        json_get_string(body, "recipient_name", NULL),
        json_get_string(body, "phone", NULL),
        json_get_string(body, "street_address", NULL),
        json_get_string(body, "street_address_2", NULL),
        json_get_string(body, "city", NULL),
        json_get_string(body, "state", NULL),
        json_get_string(body, "postal_code", NULL),
        json_get_string(body, "country", NULL)
    };

    PGresult *result = db_exec_params(conn, update_query, 11, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Address not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_addresses_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *address_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (address_id == NULL || !uuid_validate(address_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid address ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Delete address (only if owned by user) */
    const char *delete_query =
        "DELETE FROM user_addresses a "
        "USING users u "
        "WHERE a.id = $1 AND a.user_id = u.id AND (u.id = $2 OR u.supabase_id = $2) "
        "RETURNING a.id";
    const char *params[] = { address_id, user_id };

    PGresult *result = db_exec_params(conn, delete_query, 2, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Address not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
}

void handler_addresses_set_default(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);
    const char *address_id = http_request_get_path_param(req, "id");

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (address_id == NULL || !uuid_validate(address_id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid address ID");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get user's internal ID */
    const char *user_query = "SELECT id FROM users WHERE id = $1 OR supabase_id = $1";
    const char *user_params[] = { user_id };
    PGresult *user_result = db_exec_params(conn, user_query, 1, user_params);

    if (!db_result_ok(user_result) || !db_result_has_rows(user_result)) {
        PQclear(user_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "User not found");
        return;
    }

    const char *internal_user_id = db_result_value(user_result);
    char *user_id_copy = str_dup(internal_user_id);
    PQclear(user_result);

    /* Begin transaction */
    if (!db_begin(conn)) {
        free(user_id_copy);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* Unset all defaults for user */
    const char *unset_query =
        "UPDATE user_addresses SET is_default = false WHERE user_id = $1";
    const char *unset_params[] = { user_id_copy };
    PGresult *unset_result = db_exec_params(conn, unset_query, 1, unset_params);
    PQclear(unset_result);

    /* Set new default */
    const char *set_query =
        "UPDATE user_addresses SET is_default = true "
        "WHERE id = $1 AND user_id = $2 "
        "RETURNING id";
    const char *set_params[] = { address_id, user_id_copy };
    PGresult *set_result = db_exec_params(conn, set_query, 2, set_params);

    free(user_id_copy);

    if (!db_result_ok(set_result) || !db_result_has_rows(set_result)) {
        PQclear(set_result);
        db_rollback(conn);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Address not found");
        return;
    }

    PQclear(set_result);
    db_commit(conn);
    db_pool_release(pool, conn);

    http_response_success(resp, "{\"message\":\"Default address updated\"}");
}
