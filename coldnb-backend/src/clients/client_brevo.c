#include "clients/client_brevo.h"
#include "util/string_util.h"
#include "util/json_util.h"
#include "log/log.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define BREVO_API_BASE "https://api.brevo.com/v3"

/* Static configuration */
static struct {
    char *api_key;
    int list_id;
    bool sandbox_mode;
    bool initialized;
} brevo_state = {0};

/* CURL write callback */
typedef struct {
    char *data;
    size_t size;
} CurlBuffer;

static size_t brevo_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
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

int brevo_init(const BrevoConfig *config) {
    if (config == NULL || config->api_key == NULL) {
        LOG_ERROR("Brevo config is NULL or missing api_key");
        return -1;
    }

    brevo_shutdown();

    brevo_state.api_key = str_dup(config->api_key);
    if (brevo_state.api_key == NULL) {
        return -1;
    }

    brevo_state.list_id = config->list_id > 0 ? config->list_id : 1;
    brevo_state.sandbox_mode = config->sandbox_mode;
    brevo_state.initialized = true;

    LOG_INFO("Brevo client initialized (list_id: %d, sandbox: %s)",
             brevo_state.list_id,
             brevo_state.sandbox_mode ? "enabled" : "disabled");
    return 0;
}

void brevo_shutdown(void) {
    if (brevo_state.api_key != NULL) {
        memset(brevo_state.api_key, 0, strlen(brevo_state.api_key));
        free(brevo_state.api_key);
    }
    memset(&brevo_state, 0, sizeof(brevo_state));
}

bool brevo_is_initialized(void) {
    return brevo_state.initialized;
}

int brevo_get_list_id(void) {
    return brevo_state.list_id;
}

/* Make HTTP request to Brevo API */
static char *brevo_api_request(const char *method, const char *endpoint,
                               const char *body, long *http_code_out) {
    if (!brevo_state.initialized) {
        LOG_ERROR("Brevo not initialized");
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return NULL;
    }

    char *url = str_printf("%s%s", BREVO_API_BASE, endpoint);
    if (url == NULL) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    CurlBuffer response = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, brevo_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "coldnb-server/1.0");

    /* Set headers */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    char *api_key_header = str_printf("api-key: %s", brevo_state.api_key);
    headers = curl_slist_append(headers, api_key_header);
    free(api_key_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    /* Set method */
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (body != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        }
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (body != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        }
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code_out != NULL) {
        *http_code_out = http_code;
    }

    curl_slist_free_all(headers);
    free(url);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Brevo API request failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    return response.data;
}

static void brevo_log_http_error(const char *operation, long http_code, const char *response) {
    char *safe_response = str_sanitize_log(response);
    LOG_ERROR("Brevo %s failed (HTTP %ld)%s%s",
              operation ? operation : "request",
              http_code,
              safe_response ? ": " : "",
              safe_response ? safe_response : "");
    free(safe_response);
}

int brevo_add_contact(const char *email, const char *name) {
    if (!brevo_state.initialized) {
        return -1;
    }

    if (email == NULL || email[0] == '\0') {
        return -1;
    }

    /* Build request body */
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "email", email);

    if (name != NULL && name[0] != '\0') {
        cJSON *attributes = cJSON_CreateObject();
        cJSON_AddStringToObject(attributes, "FIRSTNAME", name);
        cJSON_AddItemToObject(body, "attributes", attributes);
    }

    cJSON *list_ids = cJSON_CreateArray();
    cJSON_AddItemToArray(list_ids, cJSON_CreateNumber(brevo_state.list_id));
    cJSON_AddItemToObject(body, "listIds", list_ids);

    cJSON_AddBoolToObject(body, "updateEnabled", true);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (body_str == NULL) {
        return -1;
    }

    long http_code = 0;
    char *response = brevo_api_request("POST", "/contacts", body_str, &http_code);
    free(body_str);

    /* 201 = created, 204 = updated */
    if (http_code == 201 || http_code == 204) {
        LOG_INFO("Contact added to Brevo: %s", email);
        free(response);
        return 0;
    }

    brevo_log_http_error("add contact", http_code, response);
    free(response);
    return -1;
}

int brevo_remove_contact(const char *email) {
    if (!brevo_state.initialized) {
        return -1;
    }

    if (email == NULL || email[0] == '\0') {
        return -1;
    }

    /* URL encode email */
    char *encoded_email = str_url_encode(email);
    if (encoded_email == NULL) {
        return -1;
    }

    /* Remove from list (don't delete contact entirely) */
    char *endpoint = str_printf("/contacts/lists/%d/contacts/remove", brevo_state.list_id);
    if (endpoint == NULL) {
        free(encoded_email);
        return -1;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON *emails = cJSON_CreateArray();
    cJSON_AddItemToArray(emails, cJSON_CreateString(email));
    cJSON_AddItemToObject(body, "emails", emails);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    free(encoded_email);

    if (body_str == NULL) {
        free(endpoint);
        return -1;
    }

    long http_code = 0;
    char *response = brevo_api_request("POST", endpoint, body_str, &http_code);
    free(endpoint);
    free(body_str);

    if (http_code == 201 || http_code == 204 || http_code == 200) {
        LOG_INFO("Contact removed from Brevo list: %s", email);
        free(response);
        return 0;
    }

    brevo_log_http_error("remove contact", http_code, response);
    free(response);
    return -1;
}

int brevo_update_contact(const char *email, const char *name) {
    if (!brevo_state.initialized) {
        return -1;
    }

    if (email == NULL || email[0] == '\0') {
        return -1;
    }

    char *encoded_email = str_url_encode(email);
    if (encoded_email == NULL) {
        return -1;
    }

    char *endpoint = str_printf("/contacts/%s", encoded_email);
    free(encoded_email);

    if (endpoint == NULL) {
        return -1;
    }

    cJSON *body = cJSON_CreateObject();
    if (name != NULL && name[0] != '\0') {
        cJSON *attributes = cJSON_CreateObject();
        cJSON_AddStringToObject(attributes, "FIRSTNAME", name);
        cJSON_AddItemToObject(body, "attributes", attributes);
    }

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    if (body_str == NULL) {
        free(endpoint);
        return -1;
    }

    long http_code = 0;
    char *response = brevo_api_request("PUT", endpoint, body_str, &http_code);
    free(endpoint);
    free(body_str);

    if (http_code == 204 || http_code == 200) {
        LOG_INFO("Contact updated in Brevo: %s", email);
        free(response);
        return 0;
    }

    brevo_log_http_error("update contact", http_code, response);
    free(response);
    return -1;
}

bool brevo_contact_exists(const char *email) {
    if (!brevo_state.initialized) {
        return false;
    }

    if (email == NULL || email[0] == '\0') {
        return false;
    }

    char *encoded_email = str_url_encode(email);
    if (encoded_email == NULL) {
        return false;
    }

    char *endpoint = str_printf("/contacts/%s", encoded_email);
    free(encoded_email);

    if (endpoint == NULL) {
        return false;
    }

    long http_code = 0;
    char *response = brevo_api_request("GET", endpoint, NULL, &http_code);
    free(endpoint);
    free(response);

    return http_code == 200;
}

int brevo_send_email(const BrevoEmailRequest *request, char **message_id_out) {
    if (message_id_out != NULL) {
        *message_id_out = NULL;
    }

    if (!brevo_state.initialized || request == NULL) {
        return -1;
    }

    if (str_is_empty(request->sender_email) ||
        str_is_empty(request->to_email) ||
        str_is_empty(request->subject) ||
        (str_is_empty(request->html_content) && str_is_empty(request->text_content))) {
        LOG_ERROR("Brevo send email request missing required fields");
        return -1;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON *sender = cJSON_CreateObject();
    cJSON *to_array = cJSON_CreateArray();
    cJSON *to = cJSON_CreateObject();

    if (body == NULL || sender == NULL || to_array == NULL || to == NULL) {
        cJSON_Delete(body);
        cJSON_Delete(sender);
        cJSON_Delete(to_array);
        cJSON_Delete(to);
        return -1;
    }

    cJSON_AddStringToObject(sender, "email", request->sender_email);
    if (!str_is_empty(request->sender_name)) {
        cJSON_AddStringToObject(sender, "name", request->sender_name);
    }
    cJSON_AddItemToObject(body, "sender", sender);

    cJSON_AddStringToObject(to, "email", request->to_email);
    if (!str_is_empty(request->to_name)) {
        cJSON_AddStringToObject(to, "name", request->to_name);
    }
    cJSON_AddItemToArray(to_array, to);
    cJSON_AddItemToObject(body, "to", to_array);

    cJSON_AddStringToObject(body, "subject", request->subject);

    if (!str_is_empty(request->html_content)) {
        cJSON_AddStringToObject(body, "htmlContent", request->html_content);
    }
    if (!str_is_empty(request->text_content)) {
        cJSON_AddStringToObject(body, "textContent", request->text_content);
    }

    if (!str_is_empty(request->reply_to_email)) {
        cJSON *reply_to = cJSON_CreateObject();
        if (reply_to == NULL) {
            cJSON_Delete(body);
            return -1;
        }
        cJSON_AddStringToObject(reply_to, "email", request->reply_to_email);
        if (!str_is_empty(request->reply_to_name)) {
            cJSON_AddStringToObject(reply_to, "name", request->reply_to_name);
        }
        cJSON_AddItemToObject(body, "replyTo", reply_to);
    }

    if (!str_is_empty(request->tag)) {
        cJSON *tags = cJSON_CreateArray();
        if (tags == NULL) {
            cJSON_Delete(body);
            return -1;
        }
        cJSON_AddItemToArray(tags, cJSON_CreateString(request->tag));
        cJSON_AddItemToObject(body, "tags", tags);
    }

    if (brevo_state.sandbox_mode) {
        cJSON *headers = cJSON_CreateObject();
        if (headers == NULL) {
            cJSON_Delete(body);
            return -1;
        }
        cJSON_AddStringToObject(headers, "X-Sib-Sandbox", "drop");
        cJSON_AddItemToObject(body, "headers", headers);
    }

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (body_str == NULL) {
        return -1;
    }

    long http_code = 0;
    char *response = brevo_api_request("POST", "/smtp/email", body_str, &http_code);
    free(body_str);

    if (response == NULL) {
        return -1;
    }

    if (http_code != 201 && http_code != 200) {
        brevo_log_http_error("send email", http_code, response);
        free(response);
        return -1;
    }

    cJSON *json = json_parse(response);
    if (json != NULL && message_id_out != NULL) {
        const char *message_id = json_get_string(json, "messageId", NULL);
        if (!str_is_empty(message_id)) {
            *message_id_out = str_dup(message_id);
        }
    }
    cJSON_Delete(json);

    if (message_id_out != NULL && *message_id_out != NULL) {
        LOG_INFO("Brevo transactional email queued for %s (%s)",
                 request->to_email, *message_id_out);
    } else {
        LOG_INFO("Brevo transactional email queued for %s", request->to_email);
    }

    free(response);
    return 0;
}
