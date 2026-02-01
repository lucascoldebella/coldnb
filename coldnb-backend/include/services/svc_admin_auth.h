#ifndef COLDNB_SVC_ADMIN_AUTH_H
#define COLDNB_SVC_ADMIN_AUTH_H

#include "db/db_connection.h"
#include <stdbool.h>
#include <time.h>

/* Admin user information */
typedef struct {
    char *id;           /* UUID */
    char *username;
    char *email;
    char *full_name;
    char *role;         /* admin, superadmin */
    bool is_active;
    time_t last_login;
} AdminUser;

/* Admin session information */
typedef struct {
    char *session_id;   /* UUID */
    char *admin_id;     /* UUID */
    char *token;        /* JWT token */
    time_t expires_at;
} AdminSession;

/* Configuration for admin auth */
typedef struct {
    const char *jwt_secret;
    int session_duration;   /* Seconds */
} AdminAuthConfig;

/* Initialize admin auth system
 * Returns 0 on success, -1 on error */
int admin_auth_init(const AdminAuthConfig *config);

/* Shutdown admin auth system */
void admin_auth_shutdown(void);

/* Authenticate admin by username and password
 * Returns AdminSession on success, NULL on failure
 * Caller must free with admin_session_free */
AdminSession *admin_auth_login(DbPool *pool, const char *username,
                               const char *password, const char *ip_address,
                               const char *user_agent);

/* Validate admin JWT token
 * Returns AdminUser on success, NULL on failure
 * Caller must free with admin_user_free */
AdminUser *admin_auth_validate_token(DbPool *pool, const char *token);

/* Logout admin (invalidate session)
 * Returns 0 on success, -1 on error */
int admin_auth_logout(DbPool *pool, const char *token);

/* Get admin user by ID
 * Returns AdminUser on success, NULL on failure
 * Caller must free with admin_user_free */
AdminUser *admin_auth_get_user(DbPool *pool, const char *admin_id);

/* Update admin last login timestamp */
int admin_auth_update_last_login(DbPool *pool, const char *admin_id);

/* Clean up expired sessions */
int admin_auth_cleanup_sessions(DbPool *pool);

/* Free admin user */
void admin_user_free(AdminUser *user);

/* Free admin session */
void admin_session_free(AdminSession *session);

#endif /* COLDNB_SVC_ADMIN_AUTH_H */
