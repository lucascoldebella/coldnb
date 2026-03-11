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
    char *service_role_key;
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

    if (config->service_role_key != NULL) {
        supabase_state.service_role_key = str_dup(config->service_role_key);
    }

    supabase_state.initialized = true;
    LOG_INFO("Supabase auth initialized");
    return 0;
}

void supabase_shutdown(void) {
    free(supabase_state.project_url);
    free(supabase_state.jwt_secret);
    free(supabase_state.anon_key);
    free(supabase_state.service_role_key);
    memset(&supabase_state, 0, sizeof(supabase_state));
}

static bool json_has_truthy_value(const cJSON *obj, const char *key) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (item == NULL || cJSON_IsNull(item)) {
        return false;
    }

    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }

    if (cJSON_IsString(item)) {
        return item->valuestring != NULL && item->valuestring[0] != '\0';
    }

    if (cJSON_IsNumber(item)) {
        return item->valuedouble != 0.0;
    }

    return false;
}

static SupabaseUser *supabase_user_from_json(const cJSON *json) {
    if (json == NULL || !cJSON_IsObject(json)) {
        return NULL;
    }

    const char *user_id = json_get_string(json, "id", NULL);
    if (user_id == NULL) {
        return NULL;
    }

    SupabaseUser *user = calloc(1, sizeof(SupabaseUser));
    if (user == NULL) {
        return NULL;
    }

    user->user_id = str_dup(user_id);
    user->email = str_dup(json_get_string(json, "email", NULL));
    user->phone = str_dup(json_get_string(json, "phone", NULL));
    user->role = str_dup(json_get_string(json, "role", NULL));
    user->email_verified =
        json_has_truthy_value(json, "email_confirmed_at") ||
        json_get_bool(json, "email_verified", false);
    user->phone_verified =
        json_has_truthy_value(json, "phone_confirmed_at") ||
        json_get_bool(json, "phone_verified", false);

    cJSON *app_metadata = json_get_object(json, "app_metadata");
    if (app_metadata != NULL) {
        user->provider = str_dup(json_get_string(app_metadata, "provider", NULL));
    }

    cJSON *user_metadata = json_get_object(json, "user_metadata");
    if (user_metadata != NULL) {
        user->full_name = str_dup(json_get_string(user_metadata, "full_name",
            json_get_string(user_metadata, "name", NULL)));

        if (user->phone == NULL) {
            user->phone = str_dup(json_get_string(user_metadata, "phone", NULL));
        }
    }

    return user;
}

SupabaseUser *supabase_validate_token(const char *token) {
    if (!supabase_state.initialized) {
        LOG_ERROR("Supabase not initialized");
        return NULL;
    }

    if (token == NULL || token[0] == '\0') {
        return NULL;
    }

    if (jwt_verify_hs256(token, supabase_state.jwt_secret)) {
        JwtToken *jwt = jwt_parse(token);
        if (jwt == NULL || !jwt->valid) {
            LOG_DEBUG("JWT parsing failed: %s", jwt ? jwt->error : "unknown");
            jwt_token_free(jwt);
            return NULL;
        }

        if (jwt_is_expired(jwt)) {
            LOG_DEBUG("JWT is expired");
            jwt_token_free(jwt);
            return NULL;
        }

        SupabaseUser *user = calloc(1, sizeof(SupabaseUser));
        if (user == NULL) {
            jwt_token_free(jwt);
            return NULL;
        }

        const char *sub = jwt_get_sub(jwt);
        if (sub != NULL) {
            user->user_id = str_dup(sub);
        }

        const char *email = jwt_get_string(jwt, "email");
        if (email != NULL) {
            user->email = str_dup(email);
        }

        const char *phone = jwt_get_string(jwt, "phone");
        if (phone != NULL) {
            user->phone = str_dup(phone);
        }

        cJSON *app_metadata = jwt_get_object(jwt, "app_metadata");
        if (app_metadata != NULL) {
            const char *provider = json_get_string(app_metadata, "provider", NULL);
            if (provider != NULL) {
                user->provider = str_dup(provider);
            }
        }

        cJSON *user_metadata = jwt_get_object(jwt, "user_metadata");
        if (user_metadata != NULL) {
            const char *full_name = json_get_string(user_metadata, "full_name",
                json_get_string(user_metadata, "name", NULL));
            if (full_name != NULL) {
                user->full_name = str_dup(full_name);
            }

            if (user->phone == NULL) {
                const char *metadata_phone = json_get_string(user_metadata, "phone", NULL);
                if (metadata_phone != NULL) {
                    user->phone = str_dup(metadata_phone);
                }
            }
        }

        user->email_verified =
            json_has_truthy_value(jwt->payload, "email_confirmed_at") ||
            jwt_get_bool(jwt, "email_verified");
        user->phone_verified =
            json_has_truthy_value(jwt->payload, "phone_confirmed_at") ||
            jwt_get_bool(jwt, "phone_verified");

        const char *role = jwt_get_string(jwt, "role");
        if (role != NULL) {
            user->role = str_dup(role);
        }

        jwt_token_free(jwt);
        return user;
    }

    LOG_INFO("Local JWT verification failed, falling back to Supabase user API");

    char *user_json_str = supabase_get_user_api(token);
    if (user_json_str == NULL) {
        return NULL;
    }

    cJSON *user_json = json_parse(user_json_str);
    free(user_json_str);

    if (user_json == NULL) {
        return NULL;
    }

    SupabaseUser *user = supabase_user_from_json(user_json);
    cJSON_Delete(user_json);
    return user;
}

void supabase_user_free(SupabaseUser *user) {
    if (user == NULL) {
        return;
    }
    free(user->user_id);
    free(user->email);
    free(user->phone);
    free(user->full_name);
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
static char *supabase_api_request_internal(const char *method, const char *endpoint,
                                           const char *body, const char *api_key,
                                           const char *bearer_token, long *status_out) {
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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    /* Set method */
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (strcmp(method, "GET") != 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    }

    /* Set headers */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (api_key != NULL) {
        char *apikey_header = str_printf("apikey: %s", api_key);
        headers = curl_slist_append(headers, apikey_header);
        free(apikey_header);
    }

    if (bearer_token != NULL) {
        char *auth_header = str_printf("Authorization: Bearer %s", bearer_token);
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
    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    curl_slist_free_all(headers);
    free(url);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Supabase API request failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    if (status_out != NULL) {
        *status_out = status_code;
    }

    if (status_code >= 400) {
        LOG_WARN("Supabase API request returned HTTP %ld for %s %s", status_code, method, endpoint);
        free(response.data);
        return NULL;
    }

    if (response.data == NULL) {
        return str_dup("");
    }

    return response.data;
}

static char *supabase_api_request(const char *method, const char *endpoint,
                                  const char *body, const char *access_token) {
    return supabase_api_request_internal(method, endpoint, body,
                                         supabase_state.anon_key, access_token, NULL);
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

int supabase_delete_user_admin(const char *user_id) {
    if (user_id == NULL || user_id[0] == '\0') {
        return -1;
    }

    if (!supabase_state.initialized || supabase_state.project_url == NULL ||
        supabase_state.service_role_key == NULL || supabase_state.service_role_key[0] == '\0') {
        LOG_WARN("Supabase admin delete requested without service role key configured");
        return -2;
    }

    char *endpoint = str_printf("/auth/v1/admin/users/%s", user_id);
    if (endpoint == NULL) {
        return -1;
    }

    long status_code = 0;
    char *response = supabase_api_request_internal("DELETE", endpoint, NULL,
                                                   supabase_state.service_role_key,
                                                   supabase_state.service_role_key,
                                                   &status_code);
    free(endpoint);
    free(response);

    if (status_code == 200 || status_code == 204) {
        return 0;
    }

    return -1;
}
