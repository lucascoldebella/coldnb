#include "services/svc_admin_auth.h"
#include "services/svc_jwt.h"
#include "db/db_query.h"
#include "util/hash_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Static configuration */
static struct {
    char *jwt_secret;
    int session_duration;
    bool initialized;
} admin_auth_state = {0};

int admin_auth_init(const AdminAuthConfig *config) {
    if (config == NULL || config->jwt_secret == NULL) {
        LOG_ERROR("Admin auth config is NULL or missing jwt_secret");
        return -1;
    }

    admin_auth_shutdown();

    admin_auth_state.jwt_secret = str_dup(config->jwt_secret);
    if (admin_auth_state.jwt_secret == NULL) {
        return -1;
    }

    admin_auth_state.session_duration = config->session_duration > 0 ?
                                        config->session_duration : 86400;
    admin_auth_state.initialized = true;

    LOG_INFO("Admin auth initialized (session duration: %d seconds)",
             admin_auth_state.session_duration);
    return 0;
}

void admin_auth_shutdown(void) {
    if (admin_auth_state.jwt_secret != NULL) {
        memset(admin_auth_state.jwt_secret, 0, strlen(admin_auth_state.jwt_secret));
        free(admin_auth_state.jwt_secret);
    }
    memset(&admin_auth_state, 0, sizeof(admin_auth_state));
}

void admin_user_free(AdminUser *user) {
    if (user == NULL) {
        return;
    }
    free(user->id);
    free(user->username);
    free(user->email);
    free(user->full_name);
    free(user->role);
    free(user);
}

void admin_session_free(AdminSession *session) {
    if (session == NULL) {
        return;
    }
    free(session->session_id);
    free(session->admin_id);
    if (session->token != NULL) {
        memset(session->token, 0, strlen(session->token));
        free(session->token);
    }
    free(session);
}

/* Extract AdminUser from database row */
static AdminUser *admin_user_from_row(DbRow *row) {
    AdminUser *user = calloc(1, sizeof(AdminUser));
    if (user == NULL) {
        return NULL;
    }

    const char *id = db_row_get_string(row, "id");
    const char *username = db_row_get_string(row, "username");
    const char *email = db_row_get_string(row, "email");
    const char *full_name = db_row_get_string(row, "full_name");
    const char *role = db_row_get_string(row, "role");

    user->id = id ? str_dup(id) : NULL;
    user->username = username ? str_dup(username) : NULL;
    user->email = email ? str_dup(email) : NULL;
    user->full_name = full_name ? str_dup(full_name) : NULL;
    user->role = role ? str_dup(role) : NULL;
    user->is_active = db_row_get_bool(row, "is_active");

    return user;
}

AdminSession *admin_auth_login(DbPool *pool, const char *username,
                               const char *password, const char *ip_address,
                               const char *user_agent) {
    if (!admin_auth_state.initialized) {
        LOG_ERROR("Admin auth not initialized");
        return NULL;
    }

    if (username == NULL || password == NULL) {
        return NULL;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        LOG_ERROR("Failed to acquire database connection");
        return NULL;
    }

    /* Find admin user by username */
    const char *query =
        "SELECT id, username, email, full_name, role, password_hash, is_active "
        "FROM admin_users WHERE username = $1";
    const char *params[] = { username };

    PGresult *result = db_exec_params(conn, query, 1, params);
    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        LOG_DEBUG("Admin user not found: %s", username);
        return NULL;
    }

    DbRow row = { .result = result, .row = 0 };
    const char *password_hash = db_row_get_string(&row, "password_hash");
    bool is_active = db_row_get_bool(&row, "is_active");

    if (!is_active) {
        PQclear(result);
        db_pool_release(pool, conn);
        LOG_WARN("Admin user disabled: %s", username);
        return NULL;
    }

    /* Verify password */
    if (!hash_password_verify(password, password_hash)) {
        PQclear(result);
        db_pool_release(pool, conn);
        LOG_DEBUG("Invalid password for admin: %s", username);
        return NULL;
    }

    /* Extract user info */
    AdminUser *user = admin_user_from_row(&row);
    PQclear(result);

    if (user == NULL) {
        db_pool_release(pool, conn);
        return NULL;
    }

    /* Create JWT token */
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "admin_id", user->id);
    cJSON_AddStringToObject(payload, "username", user->username);
    cJSON_AddStringToObject(payload, "role", user->role);
    cJSON_AddStringToObject(payload, "type", "admin");

    char *token = jwt_create_hs256(payload, admin_auth_state.jwt_secret,
                                   admin_auth_state.session_duration);
    cJSON_Delete(payload);

    if (token == NULL) {
        admin_user_free(user);
        db_pool_release(pool, conn);
        LOG_ERROR("Failed to create admin JWT");
        return NULL;
    }

    /* Create session record */
    char *token_hash = hash_sha256_string(token);
    if (token_hash == NULL) {
        free(token);
        admin_user_free(user);
        db_pool_release(pool, conn);
        return NULL;
    }

    time_t expires_at = time(NULL) + admin_auth_state.session_duration;
    char expires_str[32];
    snprintf(expires_str, sizeof(expires_str), "%ld", (long)expires_at);

    const char *insert_query =
        "INSERT INTO admin_sessions (admin_id, token_hash, ip_address, user_agent, expires_at) "
        "VALUES ($1, $2, $3::inet, $4, to_timestamp($5)) "
        "RETURNING id";
    const char *insert_params[] = {
        user->id,
        token_hash,
        ip_address,
        user_agent,
        expires_str
    };

    PGresult *insert_result = db_exec_params(conn, insert_query, 5, insert_params);
    free(token_hash);

    if (!db_result_ok(insert_result) || !db_result_has_rows(insert_result)) {
        PQclear(insert_result);
        free(token);
        admin_user_free(user);
        db_pool_release(pool, conn);
        LOG_ERROR("Failed to create admin session");
        return NULL;
    }

    const char *session_id = db_result_value(insert_result);

    /* Update last login */
    const char *update_query =
        "UPDATE admin_users SET last_login = NOW() WHERE id = $1";
    const char *update_params[] = { user->id };
    PGresult *update_result = db_exec_params(conn, update_query, 1, update_params);
    PQclear(update_result);

    /* Create session object */
    AdminSession *session = calloc(1, sizeof(AdminSession));
    if (session == NULL) {
        PQclear(insert_result);
        free(token);
        admin_user_free(user);
        db_pool_release(pool, conn);
        return NULL;
    }

    session->session_id = str_dup(session_id);
    session->admin_id = str_dup(user->id);
    session->token = token;
    session->expires_at = expires_at;

    PQclear(insert_result);
    admin_user_free(user);
    db_pool_release(pool, conn);

    LOG_INFO("Admin login successful: %s", username);
    return session;
}

AdminUser *admin_auth_validate_token(DbPool *pool, const char *token) {
    if (!admin_auth_state.initialized) {
        LOG_ERROR("Admin auth not initialized");
        return NULL;
    }

    if (token == NULL || token[0] == '\0') {
        return NULL;
    }

    /* Verify JWT signature */
    if (!jwt_verify_hs256(token, admin_auth_state.jwt_secret)) {
        LOG_DEBUG("Admin JWT signature verification failed");
        return NULL;
    }

    /* Parse token */
    JwtToken *jwt = jwt_parse(token);
    if (jwt == NULL || !jwt->valid) {
        LOG_DEBUG("Admin JWT parsing failed");
        jwt_token_free(jwt);
        return NULL;
    }

    /* Check expiration */
    if (jwt_is_expired(jwt)) {
        LOG_DEBUG("Admin JWT is expired");
        jwt_token_free(jwt);
        return NULL;
    }

    /* Check token type */
    const char *type = jwt_get_string(jwt, "type");
    if (type == NULL || strcmp(type, "admin") != 0) {
        LOG_DEBUG("Invalid token type for admin auth");
        jwt_token_free(jwt);
        return NULL;
    }

    /* Get admin ID */
    const char *admin_id = jwt_get_string(jwt, "admin_id");
    if (admin_id == NULL) {
        jwt_token_free(jwt);
        return NULL;
    }

    /* Verify session exists in database */
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        jwt_token_free(jwt);
        return NULL;
    }

    char *token_hash = hash_sha256_string(token);
    if (token_hash == NULL) {
        jwt_token_free(jwt);
        db_pool_release(pool, conn);
        return NULL;
    }

    const char *session_query =
        "SELECT id FROM admin_sessions "
        "WHERE admin_id = $1 AND token_hash = $2 AND expires_at > NOW()";
    const char *session_params[] = { admin_id, token_hash };

    PGresult *session_result = db_exec_params(conn, session_query, 2, session_params);
    free(token_hash);

    if (!db_result_ok(session_result) || !db_result_has_rows(session_result)) {
        PQclear(session_result);
        jwt_token_free(jwt);
        db_pool_release(pool, conn);
        LOG_DEBUG("Admin session not found or expired");
        return NULL;
    }
    PQclear(session_result);

    /* Get admin user */
    AdminUser *user = admin_auth_get_user(pool, admin_id);
    jwt_token_free(jwt);
    db_pool_release(pool, conn);

    if (user != NULL && !user->is_active) {
        admin_user_free(user);
        return NULL;
    }

    return user;
}

int admin_auth_logout(DbPool *pool, const char *token) {
    if (token == NULL) {
        return -1;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        return -1;
    }

    char *token_hash = hash_sha256_string(token);
    if (token_hash == NULL) {
        db_pool_release(pool, conn);
        return -1;
    }

    const char *query = "DELETE FROM admin_sessions WHERE token_hash = $1";
    const char *params[] = { token_hash };

    PGresult *result = db_exec_params(conn, query, 1, params);
    free(token_hash);

    bool success = db_result_ok(result);
    PQclear(result);
    db_pool_release(pool, conn);

    return success ? 0 : -1;
}

AdminUser *admin_auth_get_user(DbPool *pool, const char *admin_id) {
    if (admin_id == NULL) {
        return NULL;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        return NULL;
    }

    const char *query =
        "SELECT id, username, email, full_name, role, is_active "
        "FROM admin_users WHERE id = $1";
    const char *params[] = { admin_id };

    PGresult *result = db_exec_params(conn, query, 1, params);
    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        return NULL;
    }

    DbRow row = { .result = result, .row = 0 };
    AdminUser *user = admin_user_from_row(&row);

    PQclear(result);
    db_pool_release(pool, conn);

    return user;
}

int admin_auth_update_last_login(DbPool *pool, const char *admin_id) {
    if (admin_id == NULL) {
        return -1;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        return -1;
    }

    const char *query = "UPDATE admin_users SET last_login = NOW() WHERE id = $1";
    const char *params[] = { admin_id };

    PGresult *result = db_exec_params(conn, query, 1, params);
    bool success = db_result_ok(result);
    PQclear(result);
    db_pool_release(pool, conn);

    return success ? 0 : -1;
}

int admin_auth_cleanup_sessions(DbPool *pool) {
    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        return -1;
    }

    const char *query = "DELETE FROM admin_sessions WHERE expires_at < NOW()";
    PGresult *result = db_exec(conn, query);

    bool success = db_result_ok(result);
    int deleted = db_result_affected(result);
    PQclear(result);
    db_pool_release(pool, conn);

    if (success && deleted > 0) {
        LOG_INFO("Cleaned up %d expired admin sessions", deleted);
    }

    return success ? 0 : -1;
}
