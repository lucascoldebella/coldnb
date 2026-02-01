#include "handlers/handler_newsletter.h"
#include "clients/client_brevo.h"
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
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(email_lower);
        free(name_copy);
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

    LOG_INFO("Newsletter subscription: %s", email);
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
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(email_lower);
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
        LOG_INFO("Newsletter unsubscription: %s", email);
    }
}
