#include "services/svc_jwt.h"
#include "util/hash_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <openssl/hmac.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Split JWT into parts */
static char **jwt_split(const char *token) {
    if (token == NULL) {
        return NULL;
    }

    char **parts = calloc(4, sizeof(char *));  /* 3 parts + NULL terminator */
    if (parts == NULL) {
        return NULL;
    }

    char *token_copy = str_dup(token);
    if (token_copy == NULL) {
        free(parts);
        return NULL;
    }

    int i = 0;
    char *saveptr;
    char *part = strtok_r(token_copy, ".", &saveptr);

    while (part != NULL && i < 3) {
        parts[i] = str_dup(part);
        if (parts[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(parts[j]);
            }
            free(parts);
            free(token_copy);
            return NULL;
        }
        i++;
        part = strtok_r(NULL, ".", &saveptr);
    }

    free(token_copy);

    if (i != 3) {
        for (int j = 0; j < i; j++) {
            free(parts[j]);
        }
        free(parts);
        return NULL;
    }

    return parts;
}

static void jwt_parts_free(char **parts) {
    if (parts == NULL) {
        return;
    }
    for (int i = 0; parts[i] != NULL; i++) {
        free(parts[i]);
    }
    free(parts);
}

JwtToken *jwt_parse(const char *token) {
    JwtToken *jwt = calloc(1, sizeof(JwtToken));
    if (jwt == NULL) {
        return NULL;
    }

    if (token == NULL || token[0] == '\0') {
        jwt->valid = false;
        jwt->error = str_dup("Empty token");
        return jwt;
    }

    /* Split token */
    char **parts = jwt_split(token);
    if (parts == NULL) {
        jwt->valid = false;
        jwt->error = str_dup("Invalid token format");
        return jwt;
    }

    /* Decode header */
    size_t header_len;
    char *header_json = hash_base64url_decode(parts[0], &header_len);
    if (header_json == NULL) {
        jwt->valid = false;
        jwt->error = str_dup("Failed to decode header");
        jwt_parts_free(parts);
        return jwt;
    }

    jwt->header = cJSON_ParseWithLength(header_json, header_len);
    free(header_json);

    if (jwt->header == NULL) {
        jwt->valid = false;
        jwt->error = str_dup("Failed to parse header JSON");
        jwt_parts_free(parts);
        return jwt;
    }

    /* Decode payload */
    size_t payload_len;
    char *payload_json = hash_base64url_decode(parts[1], &payload_len);
    if (payload_json == NULL) {
        jwt->valid = false;
        jwt->error = str_dup("Failed to decode payload");
        jwt_parts_free(parts);
        return jwt;
    }

    jwt->payload = cJSON_ParseWithLength(payload_json, payload_len);
    free(payload_json);

    if (jwt->payload == NULL) {
        jwt->valid = false;
        jwt->error = str_dup("Failed to parse payload JSON");
        jwt_parts_free(parts);
        return jwt;
    }

    /* Store signature */
    jwt->signature = str_dup(parts[2]);
    jwt->valid = true;

    jwt_parts_free(parts);
    return jwt;
}

void jwt_token_free(JwtToken *jwt) {
    if (jwt == NULL) {
        return;
    }

    if (jwt->header != NULL) {
        cJSON_Delete(jwt->header);
    }
    if (jwt->payload != NULL) {
        cJSON_Delete(jwt->payload);
    }
    free(jwt->signature);
    free(jwt->error);
    free(jwt);
}

bool jwt_verify_hs256(const char *token, const char *secret) {
    if (token == NULL || secret == NULL) {
        return false;
    }

    /* Find the last dot to split header.payload from signature */
    const char *last_dot = strrchr(token, '.');
    if (last_dot == NULL) {
        return false;
    }

    /* Get the signing input (header.payload) */
    size_t input_len = (size_t)(last_dot - token);
    char *signing_input = str_ndup(token, input_len);
    if (signing_input == NULL) {
        return false;
    }

    /* Calculate expected signature */
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;

    HMAC(EVP_sha256(),
         secret, (int)strlen(secret),
         (unsigned char *)signing_input, input_len,
         hmac_result, &hmac_len);

    free(signing_input);

    /* Encode expected signature as base64url */
    char *expected = hash_base64url_encode(hmac_result, hmac_len);
    if (expected == NULL) {
        return false;
    }

    /* Compare with actual signature */
    bool valid = hash_constant_compare(expected, last_dot + 1);
    free(expected);

    return valid;
}

const char *jwt_get_string(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsString(item)) {
        return NULL;
    }

    return item->valuestring;
}

int64_t jwt_get_int(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return 0;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return 0;
    }

    return (int64_t)item->valuedouble;
}

double jwt_get_double(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return 0.0;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return 0.0;
    }

    return item->valuedouble;
}

bool jwt_get_bool(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return false;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsBool(item)) {
        return false;
    }

    return cJSON_IsTrue(item);
}

cJSON *jwt_get_object(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsObject(item)) {
        return NULL;
    }

    return item;
}

cJSON *jwt_get_array(const JwtToken *jwt, const char *claim) {
    if (jwt == NULL || jwt->payload == NULL || claim == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(jwt->payload, claim);
    if (item == NULL || !cJSON_IsArray(item)) {
        return NULL;
    }

    return item;
}

bool jwt_is_expired(const JwtToken *jwt) {
    time_t exp = jwt_get_exp(jwt);
    if (exp == 0) {
        return false;  /* No expiration set */
    }
    return time(NULL) > exp;
}

time_t jwt_get_exp(const JwtToken *jwt) {
    return (time_t)jwt_get_int(jwt, "exp");
}

time_t jwt_get_iat(const JwtToken *jwt) {
    return (time_t)jwt_get_int(jwt, "iat");
}

const char *jwt_get_sub(const JwtToken *jwt) {
    return jwt_get_string(jwt, "sub");
}

const char *jwt_get_iss(const JwtToken *jwt) {
    return jwt_get_string(jwt, "iss");
}

const char *jwt_get_aud(const JwtToken *jwt) {
    return jwt_get_string(jwt, "aud");
}

char *jwt_create_hs256(const cJSON *payload, const char *secret, int exp_seconds) {
    if (payload == NULL || secret == NULL) {
        return NULL;
    }

    /* Create header */
    cJSON *header = cJSON_CreateObject();
    if (header == NULL) {
        return NULL;
    }
    cJSON_AddStringToObject(header, "alg", "HS256");
    cJSON_AddStringToObject(header, "typ", "JWT");

    /* Clone payload and add exp if not present */
    cJSON *pay = cJSON_Duplicate(payload, true);
    if (pay == NULL) {
        cJSON_Delete(header);
        return NULL;
    }

    if (exp_seconds > 0 && !cJSON_HasObjectItem(pay, "exp")) {
        cJSON_AddNumberToObject(pay, "exp", (double)(time(NULL) + exp_seconds));
    }
    if (!cJSON_HasObjectItem(pay, "iat")) {
        cJSON_AddNumberToObject(pay, "iat", (double)time(NULL));
    }

    /* Encode header and payload */
    char *header_json = cJSON_PrintUnformatted(header);
    char *payload_json = cJSON_PrintUnformatted(pay);
    cJSON_Delete(header);
    cJSON_Delete(pay);

    if (header_json == NULL || payload_json == NULL) {
        free(header_json);
        free(payload_json);
        return NULL;
    }

    char *header_b64 = hash_base64url_encode(header_json, strlen(header_json));
    char *payload_b64 = hash_base64url_encode(payload_json, strlen(payload_json));
    free(header_json);
    free(payload_json);

    if (header_b64 == NULL || payload_b64 == NULL) {
        free(header_b64);
        free(payload_b64);
        return NULL;
    }

    /* Create signing input */
    char *signing_input = str_printf("%s.%s", header_b64, payload_b64);
    if (signing_input == NULL) {
        free(header_b64);
        free(payload_b64);
        return NULL;
    }

    /* Calculate signature */
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;

    HMAC(EVP_sha256(),
         secret, (int)strlen(secret),
         (unsigned char *)signing_input, strlen(signing_input),
         hmac_result, &hmac_len);

    char *signature = hash_base64url_encode(hmac_result, hmac_len);
    if (signature == NULL) {
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        return NULL;
    }

    /* Build final token */
    char *token = str_printf("%s.%s", signing_input, signature);

    free(header_b64);
    free(payload_b64);
    free(signing_input);
    free(signature);

    return token;
}

void jwt_add_standard_claims(cJSON *payload, const char *issuer,
                             const char *subject, int exp_seconds) {
    if (payload == NULL) {
        return;
    }

    if (issuer != NULL) {
        cJSON_AddStringToObject(payload, "iss", issuer);
    }
    if (subject != NULL) {
        cJSON_AddStringToObject(payload, "sub", subject);
    }

    time_t now = time(NULL);
    cJSON_AddNumberToObject(payload, "iat", (double)now);

    if (exp_seconds > 0) {
        cJSON_AddNumberToObject(payload, "exp", (double)(now + exp_seconds));
    }
}
