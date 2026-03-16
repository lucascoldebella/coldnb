#include "handlers/handler_newsletter.h"
#include "auth/auth_middleware.h"
#include "clients/client_brevo.h"
#include "services/svc_email.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Simple email validation */
static bool is_valid_email(const char *email) {
    if (email == NULL || strlen(email) < 5 || strlen(email) > 254) {
        return false;
    }

    const char *at = strchr(email, '@');
    if (at == NULL || at == email) {
        return false;
    }

    const char *dot = strrchr(at, '.');
    if (dot == NULL || dot == at + 1 || dot[1] == '\0') {
        return false;
    }

    return true;
}

void handler_newsletter_register(HttpRouter *router, DbPool *pool) {
    ROUTE_POST(router, "/api/newsletter/subscribe", handler_newsletter_subscribe, pool);
    ROUTE_POST(router, "/api/newsletter/unsubscribe", handler_newsletter_unsubscribe, pool);
    ROUTE_GET(router, "/api/email/unsubscribe", handler_email_unsubscribe, pool);
}

void handler_newsletter_subscribe(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *email = json_get_string(body, "email", NULL);
    const char *name = json_get_string(body, "name", NULL);

    if (str_is_empty(email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Email required");
        return;
    }

    if (!is_valid_email(email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid email format");
        return;
    }

    /* Lowercase email */
    char *email_lower = str_dup(email);
    str_to_lower(email_lower);

    /* Copy name before freeing body */
    char *name_copy = name ? str_dup(name) : NULL;

    /* Save sanitized copy for logging before freeing the JSON tree */
    char *safe_email = str_sanitize_log(email);
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(email_lower);
        free(name_copy);
        free(safe_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Check if already subscribed */
    const char *check_query =
        "SELECT id, is_active FROM newsletter_subscribers WHERE email = $1";
    const char *check_params[] = { email_lower };
    PGresult *check_result = db_exec_params(conn, check_query, 1, check_params);

    bool exists = db_result_ok(check_result) && db_result_has_rows(check_result);
    bool is_active = false;

    if (exists) {
        DbRow row = { .result = check_result, .row = 0 };
        is_active = db_row_get_bool(&row, "is_active");
    }
    PQclear(check_result);

    if (exists && is_active) {
        db_pool_release(pool, conn);
        free(email_lower);
        free(name_copy);
        free(safe_email);
        http_response_success(resp, "{\"message\":\"Already subscribed\"}");
        return;
    }

    /* Insert or update subscriber */
    const char *upsert_query =
        "INSERT INTO newsletter_subscribers (email, name, is_active, subscribed_at) "
        "VALUES ($1, $2, true, NOW()) "
        "ON CONFLICT (email) DO UPDATE SET "
        "name = COALESCE($2, newsletter_subscribers.name), "
        "is_active = true, "
        "subscribed_at = NOW(), "
        "unsubscribed_at = NULL "
        "RETURNING id";

    const char *upsert_params[] = { email_lower, name_copy };
    PGresult *upsert_result = db_exec_params(conn, upsert_query, 2, upsert_params);

    if (!db_result_ok(upsert_result)) {
        PQclear(upsert_result);
        db_pool_release(pool, conn);
        free(email_lower);
        free(name_copy);
        free(safe_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to subscribe");
        return;
    }

    PQclear(upsert_result);
    db_pool_release(pool, conn);

    /* Add to Brevo if configured */
    if (brevo_is_initialized()) {
        brevo_add_contact(email_lower, name_copy);
    }

    free(email_lower);
    free(name_copy);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", "Successfully subscribed to newsletter");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Newsletter subscription: %s", safe_email ? safe_email : "?");
    free(safe_email);
}

void handler_newsletter_unsubscribe(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *email = json_get_string(body, "email", NULL);

    if (str_is_empty(email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Email required");
        return;
    }

    /* Lowercase email */
    char *email_lower = str_dup(email);
    str_to_lower(email_lower);

    /* Save sanitized copy for logging before freeing the JSON tree */
    char *safe_email = str_sanitize_log(email);
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(email_lower);
        free(safe_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Mark as unsubscribed */
    const char *update_query =
        "UPDATE newsletter_subscribers SET "
        "is_active = false, unsubscribed_at = NOW() "
        "WHERE email = $1 AND is_active = true "
        "RETURNING id";

    const char *update_params[] = { email_lower };
    PGresult *update_result = db_exec_params(conn, update_query, 1, update_params);

    bool was_subscribed = db_result_ok(update_result) && db_result_has_rows(update_result);
    PQclear(update_result);
    db_pool_release(pool, conn);

    /* Remove from Brevo if configured */
    if (brevo_is_initialized() && was_subscribed) {
        brevo_remove_contact(email_lower);
    }

    free(email_lower);

    /* Always return success (don't reveal if email was subscribed) */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", "Successfully unsubscribed from newsletter");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    if (was_subscribed) {
        LOG_INFO("Newsletter unsubscription: %s", safe_email ? safe_email : "?");
    }
    free(safe_email);
}

void handler_email_unsubscribe(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    const char *email = http_request_get_query_param(req, "email");
    const char *token = http_request_get_query_param(req, "token");

    if (str_is_empty(email) || str_is_empty(token)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Missing email or token");
        return;
    }

    /* URL-decode the email (may contain + or %40 for @) */
    char *decoded_email = str_url_decode(email);
    if (decoded_email == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid email");
        return;
    }

    /* Verify HMAC token */
    if (!email_service_verify_unsubscribe_token(decoded_email, token)) {
        free(decoded_email);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid or expired unsubscribe link");
        return;
    }

    /* Lowercase for DB lookup */
    str_to_lower(decoded_email);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(decoded_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Mark as unsubscribed */
    const char *update_query =
        "UPDATE newsletter_subscribers SET "
        "is_active = false, unsubscribed_at = NOW() "
        "WHERE email = $1 AND is_active = true";

    const char *update_params[] = { decoded_email };
    PGresult *update_result = db_exec_params(conn, update_query, 1, update_params);
    PQclear(update_result);
    db_pool_release(pool, conn);

    /* Remove from Brevo if configured */
    if (brevo_is_initialized()) {
        brevo_remove_contact(decoded_email);
    }

    char *safe_email = str_sanitize_log(decoded_email);
    LOG_INFO("Email unsubscribe (link): %s", safe_email ? safe_email : "?");
    free(safe_email);
    free(decoded_email);

    /* Always return success (don't reveal subscription status) */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "message", "Successfully unsubscribed");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_newsletter_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/newsletter/subscribers", handler_admin_newsletter_list, pool);
    ROUTE_DELETE(router, "/api/admin/newsletter/subscribers/:id", handler_admin_newsletter_delete, pool);
}

void handler_admin_newsletter_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *status = http_request_get_query_param(req, "status");

    const char *query;
    int nparams = 0;
    const char *params[1];

    if (status && strcmp(status, "active") == 0) {
        query = "SELECT id, email, name, is_active, subscribed_at, unsubscribed_at "
                "FROM newsletter_subscribers WHERE is_active = true "
                "ORDER BY subscribed_at DESC";
    } else if (status && strcmp(status, "inactive") == 0) {
        query = "SELECT id, email, name, is_active, subscribed_at, unsubscribed_at "
                "FROM newsletter_subscribers WHERE is_active = false "
                "ORDER BY unsubscribed_at DESC NULLS LAST";
    } else {
        query = "SELECT id, email, name, is_active, subscribed_at, unsubscribed_at "
                "FROM newsletter_subscribers "
                "ORDER BY subscribed_at DESC";
    }

    PGresult *result = db_exec_params(conn, query, nparams, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *subscribers = db_result_to_json(result);
    int total = PQntuples(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "subscribers", subscribers);
    cJSON_AddNumberToObject(data, "total", total);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_newsletter_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Subscriber ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *delete_query = "DELETE FROM newsletter_subscribers WHERE id = $1";
    const char *delete_params[] = { id };
    PGresult *delete_result = db_exec_params(conn, delete_query, 1, delete_params);

    if (!db_result_ok(delete_result)) {
        PQclear(delete_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to delete subscriber");
        return;
    }

    PQclear(delete_result);
    db_pool_release(pool, conn);

    http_response_json(resp, HTTP_STATUS_OK, "{\"success\":true,\"message\":\"Subscriber deleted\"}");
}
