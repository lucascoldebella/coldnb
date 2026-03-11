#ifndef COLDNB_AUTH_SUPABASE_H
#define COLDNB_AUTH_SUPABASE_H

#include <stdbool.h>

/* User info extracted from Supabase JWT */
typedef struct {
    char *user_id;          /* Supabase user ID (UUID) */
    char *email;
    char *phone;
    char *full_name;
    bool email_verified;
    bool phone_verified;
    char *role;             /* User role from JWT */
    char *provider;         /* Auth provider (email, google, etc.) */
} SupabaseUser;

/* Configuration for Supabase auth */
typedef struct {
    const char *project_url;    /* e.g., https://xxx.supabase.co */
    const char *jwt_secret;     /* JWT signing secret */
    const char *anon_key;       /* Supabase anon key (for API calls) */
    const char *service_role_key; /* Supabase service role key (admin operations) */
} SupabaseConfig;

/* Initialize Supabase authentication
 * Returns 0 on success, -1 on error */
int supabase_init(const SupabaseConfig *config);

/* Shutdown Supabase auth */
void supabase_shutdown(void);

/* Validate a Supabase JWT token
 * Returns newly allocated SupabaseUser on success, NULL on failure
 * Caller must free with supabase_user_free */
SupabaseUser *supabase_validate_token(const char *token);

/* Free a SupabaseUser */
void supabase_user_free(SupabaseUser *user);

/* Check if token is valid without extracting user info */
bool supabase_token_valid(const char *token);

/* Get user info from Supabase API (for additional details)
 * Returns JSON string (caller must free) or NULL on error */
char *supabase_get_user_api(const char *access_token);

/* Verify email/password credentials via Supabase API
 * Returns access token (caller must free) or NULL on failure */
char *supabase_login(const char *email, const char *password);

/* Create a new user via Supabase API
 * Returns access token (caller must free) or NULL on failure */
char *supabase_signup(const char *email, const char *password);

/* Refresh an access token
 * Returns new access token (caller must free) or NULL on failure */
char *supabase_refresh_token(const char *refresh_token);

/* Delete a Supabase auth user through the admin API.
 * Returns 0 on success, -1 on request failure, -2 if admin API is not configured. */
int supabase_delete_user_admin(const char *user_id);

#endif /* COLDNB_AUTH_SUPABASE_H */
