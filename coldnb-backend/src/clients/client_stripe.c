#include "clients/client_stripe.h"
#include "util/string_util.h"
#include "util/hash_util.h"
#include "util/json_util.h"
#include "log/log.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STRIPE_API_BASE "https://api.stripe.com/v1"

/* Static configuration */
static struct {
    char *secret_key;
    char *webhook_secret;
    bool pix_enabled;
    bool initialized;
} stripe_state = {0};

/* CURL write callback */
typedef struct {
    char *data;
    size_t size;
} CurlBuffer;

static size_t stripe_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
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

int stripe_init(const StripeConfig *config) {
    if (config == NULL || config->secret_key == NULL) {
        LOG_ERROR("Stripe config is NULL or missing secret_key");
        return -1;
    }

    stripe_shutdown();

    stripe_state.secret_key = str_dup(config->secret_key);
    if (stripe_state.secret_key == NULL) {
        return -1;
    }

    if (config->webhook_secret != NULL) {
        stripe_state.webhook_secret = str_dup(config->webhook_secret);
    }

    stripe_state.pix_enabled = config->pix_enabled;
    stripe_state.initialized = true;

    LOG_INFO("Stripe client initialized (PIX %s)",
             stripe_state.pix_enabled ? "enabled" : "disabled");
    return 0;
}

void stripe_shutdown(void) {
    if (stripe_state.secret_key != NULL) {
        memset(stripe_state.secret_key, 0, strlen(stripe_state.secret_key));
        free(stripe_state.secret_key);
    }
    if (stripe_state.webhook_secret != NULL) {
        memset(stripe_state.webhook_secret, 0, strlen(stripe_state.webhook_secret));
        free(stripe_state.webhook_secret);
    }
    memset(&stripe_state, 0, sizeof(stripe_state));
}

bool stripe_is_initialized(void) {
    return stripe_state.initialized;
}

bool stripe_pix_is_enabled(void) {
    return stripe_state.initialized && stripe_state.pix_enabled;
}

void stripe_payment_intent_free(StripePaymentIntent *pi) {
    if (pi == NULL) return;
    free(pi->id);
    free(pi->client_secret);
    free(pi->status);
    free(pi->currency);
    free(pi);
}

void stripe_pix_payment_free(StripePixPayment *pix) {
    if (pix == NULL) return;
    free(pix->id);
    free(pix->qr_code);
    free(pix->qr_code_url);
    free(pix->expires_at);
    free(pix);
}

void stripe_webhook_event_free(StripeWebhookEvent *event) {
    if (event == NULL) return;
    free(event->id);
    free(event->type);
    free(event->data);
    free(event);
}

/* Make HTTP request to Stripe API */
static char *stripe_api_request(const char *method, const char *endpoint, const char *post_data) {
    if (!stripe_state.initialized) {
        LOG_ERROR("Stripe not initialized");
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return NULL;
    }

    char *url = str_printf("%s%s", STRIPE_API_BASE, endpoint);
    if (url == NULL) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    CurlBuffer response = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stripe_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERNAME, stripe_state.secret_key);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (post_data != NULL) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        }
    } else if (strcmp(method, "GET") == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    free(url);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Stripe API request failed: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    if (http_code >= 400) {
        LOG_ERROR("Stripe API error (HTTP %ld): %s", http_code, response.data);
        free(response.data);
        return NULL;
    }

    return response.data;
}

StripePaymentIntent *stripe_create_payment_intent(int64_t amount, const char *currency,
                                                   const char *description,
                                                   const char *customer_email,
                                                   const char *metadata_order_id) {
    if (!stripe_state.initialized) {
        return NULL;
    }

    /* Build POST data */
    char *post_data = str_printf(
        "amount=%lld&currency=%s&automatic_payment_methods[enabled]=true",
        (long long)amount, currency ? currency : "brl"
    );

    if (post_data == NULL) {
        return NULL;
    }

    if (description != NULL) {
        char *encoded_desc = str_url_encode(description);
        char *new_data = str_printf("%s&description=%s", post_data, encoded_desc);
        free(encoded_desc);
        free(post_data);
        post_data = new_data;
    }

    if (customer_email != NULL) {
        char *encoded_email = str_url_encode(customer_email);
        char *new_data = str_printf("%s&receipt_email=%s", post_data, encoded_email);
        free(encoded_email);
        free(post_data);
        post_data = new_data;
    }

    if (metadata_order_id != NULL) {
        char *new_data = str_printf("%s&metadata[order_id]=%s", post_data, metadata_order_id);
        free(post_data);
        post_data = new_data;
    }

    char *response = stripe_api_request("POST", "/payment_intents", post_data);
    free(post_data);

    if (response == NULL) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    StripePaymentIntent *pi = calloc(1, sizeof(StripePaymentIntent));
    if (pi == NULL) {
        cJSON_Delete(json);
        return NULL;
    }

    const char *id = json_get_string(json, "id", NULL);
    const char *client_secret = json_get_string(json, "client_secret", NULL);
    const char *status = json_get_string(json, "status", NULL);
    const char *curr = json_get_string(json, "currency", NULL);
    int64_t amt = json_get_int64(json, "amount", 0);

    pi->id = id ? str_dup(id) : NULL;
    pi->client_secret = client_secret ? str_dup(client_secret) : NULL;
    pi->status = status ? str_dup(status) : NULL;
    pi->currency = curr ? str_dup(curr) : NULL;
    pi->amount = amt;

    cJSON_Delete(json);
    return pi;
}

StripePixPayment *stripe_create_pix_payment(int64_t amount, const char *description,
                                            const char *customer_email,
                                            const char *metadata_order_id) {
    if (!stripe_pix_is_enabled()) {
        LOG_ERROR("PIX payments not enabled");
        return NULL;
    }

    /* Build POST data for PIX payment intent */
    char *post_data = str_printf(
        "amount=%lld&currency=brl&payment_method_types[]=pix",
        (long long)amount
    );

    if (post_data == NULL) {
        return NULL;
    }

    if (description != NULL) {
        char *encoded_desc = str_url_encode(description);
        char *new_data = str_printf("%s&description=%s", post_data, encoded_desc);
        free(encoded_desc);
        free(post_data);
        post_data = new_data;
    }

    if (customer_email != NULL) {
        char *encoded_email = str_url_encode(customer_email);
        char *new_data = str_printf("%s&receipt_email=%s", post_data, encoded_email);
        free(encoded_email);
        free(post_data);
        post_data = new_data;
    }

    if (metadata_order_id != NULL) {
        char *new_data = str_printf("%s&metadata[order_id]=%s", post_data, metadata_order_id);
        free(post_data);
        post_data = new_data;
    }

    char *response = stripe_api_request("POST", "/payment_intents", post_data);
    free(post_data);

    if (response == NULL) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    StripePixPayment *pix = calloc(1, sizeof(StripePixPayment));
    if (pix == NULL) {
        cJSON_Delete(json);
        return NULL;
    }

    const char *id = json_get_string(json, "id", NULL);
    pix->id = id ? str_dup(id) : NULL;

    /* PIX details are in next_action.pix_display_qr_code */
    cJSON *next_action = json_get_object(json, "next_action");
    if (next_action != NULL) {
        cJSON *pix_qr = json_get_object(next_action, "pix_display_qr_code");
        if (pix_qr != NULL) {
            const char *qr_code = json_get_string(pix_qr, "data", NULL);
            const char *qr_url = json_get_string(pix_qr, "image_url_png", NULL);
            const char *expires = json_get_string(pix_qr, "expires_at", NULL);

            pix->qr_code = qr_code ? str_dup(qr_code) : NULL;
            pix->qr_code_url = qr_url ? str_dup(qr_url) : NULL;
            pix->expires_at = expires ? str_dup(expires) : NULL;
        }
    }

    cJSON_Delete(json);
    return pix;
}

StripePaymentIntent *stripe_get_payment_intent(const char *payment_intent_id) {
    if (!stripe_state.initialized || payment_intent_id == NULL) {
        return NULL;
    }

    char *endpoint = str_printf("/payment_intents/%s", payment_intent_id);
    if (endpoint == NULL) {
        return NULL;
    }

    char *response = stripe_api_request("GET", endpoint, NULL);
    free(endpoint);

    if (response == NULL) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(response);
    free(response);

    if (json == NULL) {
        return NULL;
    }

    StripePaymentIntent *pi = calloc(1, sizeof(StripePaymentIntent));
    if (pi == NULL) {
        cJSON_Delete(json);
        return NULL;
    }

    const char *id = json_get_string(json, "id", NULL);
    const char *client_secret = json_get_string(json, "client_secret", NULL);
    const char *status = json_get_string(json, "status", NULL);
    const char *curr = json_get_string(json, "currency", NULL);
    int64_t amt = json_get_int64(json, "amount", 0);

    pi->id = id ? str_dup(id) : NULL;
    pi->client_secret = client_secret ? str_dup(client_secret) : NULL;
    pi->status = status ? str_dup(status) : NULL;
    pi->currency = curr ? str_dup(curr) : NULL;
    pi->amount = amt;

    cJSON_Delete(json);
    return pi;
}

StripeWebhookEvent *stripe_verify_webhook(const char *payload, const char *signature) {
    if (!stripe_state.initialized || stripe_state.webhook_secret == NULL) {
        LOG_ERROR("Webhook secret not configured");
        return NULL;
    }

    if (payload == NULL || signature == NULL) {
        return NULL;
    }

    /* Parse Stripe-Signature header
     * Format: t=timestamp,v1=signature,v1=signature... */
    char *timestamp_str = NULL;
    char *sig_v1 = NULL;

    char *sig_copy = str_dup(signature);
    char **parts = str_split(sig_copy, ',', 0);
    free(sig_copy);

    if (parts == NULL) {
        return NULL;
    }

    for (int i = 0; parts[i] != NULL; i++) {
        if (str_starts_with(parts[i], "t=")) {
            timestamp_str = str_dup(parts[i] + 2);
        } else if (str_starts_with(parts[i], "v1=")) {
            if (sig_v1 == NULL) {
                sig_v1 = str_dup(parts[i] + 3);
            }
        }
    }
    str_split_free(parts);

    if (timestamp_str == NULL || sig_v1 == NULL) {
        free(timestamp_str);
        free(sig_v1);
        return NULL;
    }

    /* Check timestamp (allow 5 minute tolerance) */
    long timestamp = atol(timestamp_str);
    time_t now = time(NULL);
    if (now - timestamp > 300) {
        LOG_WARN("Webhook timestamp too old: %ld", timestamp);
        free(timestamp_str);
        free(sig_v1);
        return NULL;
    }

    /* Compute expected signature: HMAC-SHA256(timestamp.payload) */
    char *signed_payload = str_printf("%s.%s", timestamp_str, payload);
    free(timestamp_str);

    if (signed_payload == NULL) {
        free(sig_v1);
        return NULL;
    }

    char *expected_sig = hash_hmac_sha256(
        stripe_state.webhook_secret, strlen(stripe_state.webhook_secret),
        signed_payload, strlen(signed_payload)
    );
    free(signed_payload);

    if (expected_sig == NULL) {
        free(sig_v1);
        return NULL;
    }

    /* Constant-time comparison */
    bool valid = hash_constant_compare(expected_sig, sig_v1);
    free(expected_sig);
    free(sig_v1);

    if (!valid) {
        LOG_WARN("Webhook signature verification failed");
        return NULL;
    }

    /* Parse event */
    cJSON *json = cJSON_Parse(payload);
    if (json == NULL) {
        return NULL;
    }

    StripeWebhookEvent *event = calloc(1, sizeof(StripeWebhookEvent));
    if (event == NULL) {
        cJSON_Delete(json);
        return NULL;
    }

    const char *id = json_get_string(json, "id", NULL);
    const char *type = json_get_string(json, "type", NULL);
    cJSON *data = json_get_object(json, "data");

    event->id = id ? str_dup(id) : NULL;
    event->type = type ? str_dup(type) : NULL;
    event->data = data ? cJSON_PrintUnformatted(data) : NULL;

    cJSON_Delete(json);
    return event;
}
