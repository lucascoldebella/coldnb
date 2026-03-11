#include "handlers/handler_contact.h"
#include "db/db_query.h"
#include "services/svc_email.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

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

void handler_contact_register(HttpRouter *router, DbPool *pool) {
    ROUTE_POST(router, "/api/contact", handler_contact_submit, pool);
}

void handler_contact_submit(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *name = json_get_string(body, "name", NULL);
    const char *email = json_get_string(body, "email", NULL);
    const char *message = json_get_string(body, "message", NULL);
    const char *phone = json_get_string(body, "phone", NULL);
    const char *subject = json_get_string(body, "subject", NULL);

    /* Validate required fields */
    if (str_is_empty(name)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name is required");
        return;
    }

    if (str_is_empty(email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Email is required");
        return;
    }

    if (!is_valid_email(email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid email format");
        return;
    }

    if (str_is_empty(message)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Message is required");
        return;
    }

    /* Validate lengths */
    if (strlen(name) > 255) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name too long");
        return;
    }

    if (strlen(message) > 10000) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Message too long");
        return;
    }

    if (phone != NULL && strlen(phone) > 50) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Phone number too long");
        return;
    }

    if (subject != NULL && strlen(subject) > 255) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Subject too long");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Insert contact submission */
    const char *insert_query =
        "INSERT INTO contact_submissions (name, email, phone, subject, message) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id";

    const char *params[] = { name, email, phone, subject, message };
    PGresult *result = db_exec_params(conn, insert_query, 5, params);

    /* Save copies before freeing the JSON tree */
    char *name_copy = str_dup(name);
    char *email_copy = str_dup(email);
    char *phone_copy = phone ? str_dup(phone) : NULL;
    char *subject_copy = subject ? str_dup(subject) : NULL;
    char *message_copy = str_dup(message);

    /* Save sanitized copies for logging before freeing the JSON tree */
    char *safe_name = str_sanitize_log(name);
    char *safe_email = str_sanitize_log(email);

    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        free(name_copy);
        free(email_copy);
        free(phone_copy);
        free(subject_copy);
        free(message_copy);
        free(safe_name);
        free(safe_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to submit contact form");
        return;
    }

    char *submission_id = str_dup(db_result_value(result));
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", submission_id);
    cJSON_AddStringToObject(data, "message", "Thank you for your message. We will get back to you soon.");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    EmailContactSubmission submission = {
        .submission_id = submission_id,
        .name = name_copy,
        .email = email_copy,
        .phone = phone_copy,
        .subject = subject_copy,
        .message = message_copy
    };
    if (email_service_send_contact_notification(&submission) != 0) {
        LOG_WARN("Contact submission email notification failed for %s",
                 safe_email ? safe_email : "?");
    }

    LOG_INFO("Contact form submission from: %s <%s>",
             safe_name ? safe_name : "?", safe_email ? safe_email : "?");
    free(submission_id);
    free(name_copy);
    free(email_copy);
    free(phone_copy);
    free(subject_copy);
    free(message_copy);
    free(safe_name);
    free(safe_email);
}
