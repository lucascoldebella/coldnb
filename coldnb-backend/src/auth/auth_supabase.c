#include "auth/auth_supabase.h"
#include "services/svc_jwt.h"
#include "util/string_util.h"
#include "util/json_util.h"
#include "log/log.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

/* Static configuration */
static struct {
    char *project_url;
    char *jwt_secret;
    char *anon_key;
    bool initialized;
} supabase_state = {0};

/* CURL write callback */
typedef struct {
    char *data;
    size_t size;
} CurlBuffer;

static size_t supabase_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    CurlBuffer *buf = (CurlBuffer *)userp;

    char *new_data = realloc(buf->data, buf->size + total + 1);
    if (new_data == NULL) {
        return 0;
    }

    buf->data = new_data;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';

    return total;
}

int supabase_init(const SupabaseConfig *config) {
    if (config == NULL) {
        LOG_ERROR("Supabase config is NULL");
        return -1;
    }

    if (config->jwt_secret == NULL) {
        LOG_ERROR("Supabase JWT secret is required");
        return -1;
    }

    /* Clean up any previous state */
    supabase_shutdown();

    if (config->project_url != NULL) {
        supabase_state.project_url = str_dup(config->project_url);
    }

    supabase_state.jwt_secret = str_dup(config->jwt_secret);
    if (supabase_state.jwt_secret == NULL) {
        supabase_shutdown();
        return -1;
    }

    if (config->anon_key != NULL) {
        supabase_state.anon_key = str_dup(config->anon_key);
    }

    supabase_state.initialized = true;
    LOG_INFO("Supabase auth initialized");
    return 0;
}

void supabase_shutdown(void) {
    free(supabase_state.project_url);
    free(supabase_state.jwt_secret);
    free(supabase_state.anon_key);
    memset(&supabase_state, 0, sizeof(supabase_state));
}

SupabaseUser *supabase_validate_token(const char *token) {
    if (!supabase_state.initialized) {
        LOG_ERROR("Supabase not initialized");
        return NULL;
    }

    if (token == NULL || token[0] == '\0') {
        return NULL;
    }

    /* Verify signature */
    if (!jwt_verify_hs256(token, supabase_state.jwt_secret)) {
        LOG_DEBUG("JWT signature verification failed");
        return NULL;
    }

    /* Parse token */
    JwtToken *jwt = jwt_parse(token);
    if (jwt == NULL || !jwt->valid) {
        LOG_DEBUG("JWT parsing failed: %s", jwt ? jwt->error : "unknown");
        jwt_token_free(jwt);
        return NULL;
    }

    /* Check expiration */
    if (jwt_is_expired(jwt)) {
        LOG_DEBUG("JWT is expired");
        jwt_token_free(jwt);
        return NULL;
    }

    /* Extract user info */
    SupabaseUser *user = calloc(1, sizeof(SupabaseUser));
    if (user == NULL) {
        jwt_token_free(jwt);
        return NULL;
    }

    /* Get subject (user ID) */
    const char *sub = jwt_get_sub(jwt);
    if (sub != NULL) {
        user->user_id = str_dup(sub);
    }

    /* Get email from payload */
    const char *email = jwt_get_string(jwt, "email");
    if (email != NULL) {
        user->email = str_dup(email);
    }

    /* Get phone */
    const char *phone = jwt_get_string(jwt, "phone");
    if (phone != NULL) {
        user->phone = str_dup(phone);
    }

    /* Get verification status from app_metadata or directly */
    cJSON *app_metadata = jwt_get_object(jwt, "app_metadata");
    if (app_metadata != NULL) {
        const char *provider = json_get_string(app_metadata, "provider", NULL);
        if (provider != NULL) {
            user->provider = str_dup(provider);
        }
    }

    /* Check email verification */
    user->email_verified = jwt_get_bool(jwt, "email_confirmed_at") ||
                           jwt_get_bool(jwt, "email_verified");

    /* Get role */
    const char *role = jwt_get_string(jwt, "role");
    if (role != NULL) {
        user->role = str_dup(role);
    }

    jwt_token_free(jwt);
    return user;
}

void supabase_user_free(SupabaseUser *user) {
    if (user == NULL) {
        return;
    }
    free(user->user_id);
    free(user->email);
    free(user->phone);
    free(user->role);
    free(user->provider);
    free(user);
}

bool supabase_token_valid(const char *token) {
    SupabaseUser *user = supabase_validate_token(token);
    if (user == NULL) {
        return false;
    }
    supabase_user_free(user);
    return true;
}

/* Make HTTP request to Supabase API */
static char *supabase_api_request(const char *method, const char *endpoint,
                                  const char *body, const char *access_token) {
    if (!supabase_state.initialized || supabase_state.project_url == NULL) {
        LOG_ERROR("Supabase not properly configured for API calls");
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return NULL;
    }

    /* Build URL */
    char *url = str_printf("%s%s", supabase_state.project_url, endpoint);
    if (url == NULL) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    CurlBuffer response = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, supabase_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    /* Set method */
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }

    /* Set headers */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (supabase_state.anon_key != NULL) {
        char *apikey_header = str_printf("apikey: %s", supabase_state.anon_key);
        headers = curl_slist_append(headers, apikey_header);
        free(apikey_header);
    }

    if (access_token != NULL) {
        char *auth_header = str_printf("Authorization: Bearer %s", access_token);
        headers = curl_slist_append(headers, auth_header);
        free(auth_header);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    /* Set body */
    if (body != NULL) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }

    /* Perform request */
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    free(url);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Supabase API request failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    return response.data;
}

char *supabase_get_user_api(const char *access_token) {
    return supabase_api_request("GET", "/auth/v1/user", NULL, access_token);
}

char *supabase_login(const char *email, const char *password) {
    if (email == NULL || password == NULL) {
        return NULL;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "email", email);
    cJSON_AddStringToObject(body, "password", password);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (body_str == NULL) {
        return NULL;
    }

    char *response = supabase_api_request("POST", "/auth/v1/token?grant_type=password",
                                          body_str, NULL);
    free(body_str);

    if (response == NULL) {
        return NULL;
    }

    /* Extract access token from response */
    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    const char *access_token = json_get_string(json, "access_token", NULL);
    char *result = access_token ? str_dup(access_token) : NULL;

    cJSON_Delete(json);
    return result;
}

char *supabase_signup(const char *email, const char *password) {
    if (email == NULL || password == NULL) {
        return NULL;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "email", email);
    cJSON_AddStringToObject(body, "password", password);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (body_str == NULL) {
        return NULL;
    }

    char *response = supabase_api_request("POST", "/auth/v1/signup", body_str, NULL);
    free(body_str);

    if (response == NULL) {
        return NULL;
    }

    /* Extract access token from response */
    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    const char *access_token = json_get_string(json, "access_token", NULL);
    char *result = access_token ? str_dup(access_token) : NULL;

    cJSON_Delete(json);
    return result;
}

char *supabase_refresh_token(const char *refresh_token) {
    if (refresh_token == NULL) {
        return NULL;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "refresh_token", refresh_token);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (body_str == NULL) {
        return NULL;
    }

    char *response = supabase_api_request("POST", "/auth/v1/token?grant_type=refresh_token",
                                          body_str, NULL);
    free(body_str);

    if (response == NULL) {
        return NULL;
    }

    /* Extract new access token from response */
    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    const char *access_token = json_get_string(json, "access_token", NULL);
    char *result = access_token ? str_dup(access_token) : NULL;

    cJSON_Delete(json);
    return result;
}
