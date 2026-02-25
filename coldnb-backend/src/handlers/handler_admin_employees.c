#include "handlers/handler_admin_employees.h"
#include "services/svc_admin_auth.h"
#include "auth/auth_middleware.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/hash_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

/* Helper to check if current user is super_admin */
static bool is_super_admin(HttpRequest *req, DbPool *pool) {
    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        return false;
    }

    AdminUser *user = admin_auth_validate_token(pool, token);
    if (user == NULL) {
        return false;
    }

    bool result = (strcmp(user->role, "super_admin") == 0);
    admin_user_free(user);
    return result;
}

/* Helper to check permission */
static bool has_permission(HttpRequest *req, DbPool *pool, const char *permission) {
    (void)permission; /* TODO: implement permission checking */
    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        return false;
    }

    AdminUser *user = admin_auth_validate_token(pool, token);
    if (user == NULL) {
        return false;
    }

    /* super_admin bypasses all permission checks */
    if (strcmp(user->role, "super_admin") == 0) {
        admin_user_free(user);
        return true;
    }

    /* TODO: Check specific permission in user->permissions JSON */
    /* For now, regular admins have limited access */
    admin_user_free(user);
    return false;
}

/* Convert PGresult row to JSON object */
static cJSON *employee_row_to_json(PGresult *res, int row) {
    cJSON *emp = cJSON_CreateObject();

    cJSON_AddStringToObject(emp, "id", PQgetvalue(res, row, 0));
    cJSON_AddStringToObject(emp, "username", PQgetvalue(res, row, 1));
    json_add_string_if(emp, "email", PQgetvalue(res, row, 2));
    json_add_string_if(emp, "full_name", PQgetvalue(res, row, 3));
    cJSON_AddStringToObject(emp, "role", PQgetvalue(res, row, 4));
    cJSON_AddBoolToObject(emp, "is_active", strcmp(PQgetvalue(res, row, 5), "t") == 0);
    json_add_string_if(emp, "employee_id", PQgetvalue(res, row, 6));
    json_add_string_if(emp, "cpf", PQgetvalue(res, row, 7));
    json_add_string_if(emp, "photo_url", PQgetvalue(res, row, 8));
    json_add_string_if(emp, "phone", PQgetvalue(res, row, 9));
    json_add_string_if(emp, "last_login", PQgetvalue(res, row, 10));
    json_add_string_if(emp, "created_at", PQgetvalue(res, row, 11));

    /* Parse permissions JSON */
    const char *perms_str = PQgetvalue(res, row, 12);
    if (perms_str != NULL && strlen(perms_str) > 0) {
        cJSON *perms = cJSON_Parse(perms_str);
        if (perms != NULL) {
            cJSON_AddItemToObject(emp, "permissions", perms);
        } else {
            cJSON_AddObjectToObject(emp, "permissions");
        }
    } else {
        cJSON_AddObjectToObject(emp, "permissions");
    }

    return emp;
}

void handler_admin_employees_register(HttpRouter *router, DbPool *pool) {
    ROUTE_GET(router, "/api/admin/employees", handler_admin_employees_list, pool);
    ROUTE_GET(router, "/api/admin/employees/:id", handler_admin_employees_get, pool);
    ROUTE_POST(router, "/api/admin/employees", handler_admin_employees_create, pool);
    ROUTE_PUT(router, "/api/admin/employees/:id", handler_admin_employees_update, pool);
    ROUTE_DELETE(router, "/api/admin/employees/:id", handler_admin_employees_delete, pool);
    ROUTE_PUT(router, "/api/admin/employees/:id/permissions", handler_admin_employees_update_permissions, pool);
    ROUTE_PUT(router, "/api/admin/employees/:id/status", handler_admin_employees_update_status, pool);
}

void handler_admin_employees_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Check permission */
    if (!has_permission(req, pool, "manage_team")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Query employees */
    const char *query =
        "SELECT id, username, email, full_name, role, is_active, "
        "employee_id, cpf, photo_url, phone, last_login, created_at, permissions "
        "FROM admin_users "
        "ORDER BY created_at DESC";

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to list employees: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Build response */
    cJSON *employees = cJSON_CreateArray();
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        cJSON_AddItemToArray(employees, employee_row_to_json(res, i));
    }

    PQclear(res);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "employees", employees);
    cJSON_AddNumberToObject(data, "total", rows);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_employees_get(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Check permission */
    if (!has_permission(req, pool, "manage_team")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Employee ID required");
        return;
    }

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Query employee */
    const char *query =
        "SELECT id, username, email, full_name, role, is_active, "
        "employee_id, cpf, photo_url, phone, last_login, created_at, permissions "
        "FROM admin_users WHERE id = $1";

    const char *params[1] = { id };
    PGresult *res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to get employee: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Employee not found");
        return;
    }

    cJSON *employee = employee_row_to_json(res, 0);
    PQclear(res);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "employee", employee);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

void handler_admin_employees_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Only super_admin can create employees */
    if (!is_super_admin(req, pool)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Only super_admin can create employees");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    /* Get required fields */
    const char *username = json_get_string(body, "username", NULL);
    const char *email = json_get_string(body, "email", NULL);
    const char *password = json_get_string(body, "password", NULL);
    const char *role = json_get_string(body, "role", "admin");

    if (str_is_empty(username) || str_is_empty(password)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Username and password are required");
        return;
    }

    /* Get optional fields */
    const char *full_name = json_get_string(body, "full_name", NULL);
    const char *employee_id = json_get_string(body, "employee_id", NULL);
    const char *cpf = json_get_string(body, "cpf", NULL);
    const char *phone = json_get_string(body, "phone", NULL);
    const char *photo_url = json_get_string(body, "photo_url", NULL);

    /* Hash password */
    char *password_hash = hash_password(password);
    if (password_hash == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to hash password");
        return;
    }

    /* Get current admin ID for created_by */
    const char *token = http_request_get_bearer_token(req);
    AdminUser *current_user = admin_auth_validate_token(pool, token);
    const char *created_by = current_user ? current_user->id : NULL;

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(password_hash);
        cJSON_Delete(body);
        if (current_user) admin_user_free(current_user);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Insert employee */
    const char *query =
        "INSERT INTO admin_users (username, email, password_hash, role, full_name, "
        "employee_id, cpf, phone, photo_url, created_by, permissions) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, '{}') "
        "RETURNING id";

    const char *params[10] = {
        username, email, password_hash, role, full_name,
        employee_id, cpf, phone, photo_url, created_by
    };

    PGresult *res = PQexecParams(conn, query, 10, NULL, params, NULL, NULL, 0);
    free(password_hash);
    if (current_user) admin_user_free(current_user);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        LOG_ERROR("Failed to create employee: %s", PQerrorMessage(conn));
        const char *error = PQerrorMessage(conn);
        PQclear(res);
        db_pool_release(pool, conn);
        cJSON_Delete(body);

        /* Check for duplicate username/email */
        if (strstr(error, "duplicate key") != NULL) {
            http_response_error(resp, HTTP_STATUS_CONFLICT, "Username or email already exists");
        } else {
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create employee");
        }
        return;
    }

    const char *new_id = PQgetvalue(res, 0, 0);

    PQclear(res);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    /* Return success with new ID */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", new_id);
    cJSON_AddStringToObject(data, "message", "Employee created successfully");

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);

    LOG_INFO("Employee created: %s", username);
}

void handler_admin_employees_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Check permission */
    if (!has_permission(req, pool, "edit_employees")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Employee ID required");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Build UPDATE query dynamically based on provided fields */
    /* For simplicity, update all allowed fields */
    const char *email = json_get_string(body, "email", NULL);
    const char *full_name = json_get_string(body, "full_name", NULL);
    const char *employee_id = json_get_string(body, "employee_id", NULL);
    const char *cpf = json_get_string(body, "cpf", NULL);
    const char *phone = json_get_string(body, "phone", NULL);
    const char *photo_url = json_get_string(body, "photo_url", NULL);

    /* Only super_admin can change role */
    const char *role = NULL;
    if (is_super_admin(req, pool)) {
        role = json_get_string(body, "role", NULL);
    }

    const char *query =
        "UPDATE admin_users SET "
        "email = COALESCE($2, email), "
        "full_name = COALESCE($3, full_name), "
        "employee_id = COALESCE($4, employee_id), "
        "cpf = COALESCE($5, cpf), "
        "phone = COALESCE($6, phone), "
        "photo_url = COALESCE($7, photo_url), "
        "role = COALESCE($8, role), "
        "updated_at = NOW() "
        "WHERE id = $1";

    const char *params[8] = { id, email, full_name, employee_id, cpf, phone, photo_url, role };
    PGresult *res = PQexecParams(conn, query, 8, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to update employee: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to update employee");
        return;
    }

    PQclear(res);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    http_response_success(resp, "{\"message\":\"Employee updated successfully\"}");
    LOG_INFO("Employee updated: %s", id);
}

void handler_admin_employees_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Only super_admin can delete employees */
    if (!is_super_admin(req, pool)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Only super_admin can delete employees");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Employee ID required");
        return;
    }

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Soft delete: set is_active = false */
    const char *query = "UPDATE admin_users SET is_active = false WHERE id = $1";
    const char *params[1] = { id };
    PGresult *res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to delete employee: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to delete employee");
        return;
    }

    PQclear(res);
    db_pool_release(pool, conn);

    http_response_success(resp, "{\"message\":\"Employee deleted successfully\"}");
    LOG_INFO("Employee deleted (soft): %s", id);
}

void handler_admin_employees_update_permissions(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Only super_admin can assign permissions */
    if (!is_super_admin(req, pool)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Only super_admin can assign permissions");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Employee ID required");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *permissions = cJSON_GetObjectItem(body, "permissions");
    if (permissions == NULL || !cJSON_IsObject(permissions)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Permissions object required");
        return;
    }

    /* Convert permissions to string */
    char *perms_str = cJSON_PrintUnformatted(permissions);
    if (perms_str == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to serialize permissions");
        return;
    }

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(perms_str);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Update permissions */
    const char *query = "UPDATE admin_users SET permissions = $2::jsonb WHERE id = $1";
    const char *params[2] = { id, perms_str };
    PGresult *res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    free(perms_str);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to update permissions: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to update permissions");
        return;
    }

    PQclear(res);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    http_response_success(resp, "{\"message\":\"Permissions updated successfully\"}");
    LOG_INFO("Permissions updated for employee: %s", id);
}

void handler_admin_employees_update_status(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    /* Only super_admin can change status */
    if (!is_super_admin(req, pool)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Only super_admin can change employee status");
        return;
    }

    const char *id = http_request_get_path_param(req, "id");
    if (str_is_empty(id)) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Employee ID required");
        return;
    }

    /* Parse request body */
    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *is_active_item = cJSON_GetObjectItem(body, "isActive");
    if (is_active_item == NULL || !cJSON_IsBool(is_active_item)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "isActive boolean required");
        return;
    }

    bool is_active = cJSON_IsTrue(is_active_item);

    /* Get database connection */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database error");
        return;
    }

    /* Update status */
    const char *query = "UPDATE admin_users SET is_active = $2 WHERE id = $1";
    const char *params[2] = { id, is_active ? "true" : "false" };
    PGresult *res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        LOG_ERROR("Failed to update status: %s", PQerrorMessage(conn));
        PQclear(res);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to update status");
        return;
    }

    PQclear(res);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    http_response_success(resp, "{\"message\":\"Status updated successfully\"}");
    LOG_INFO("Status updated for employee %s: %s", id, is_active ? "active" : "inactive");
}
