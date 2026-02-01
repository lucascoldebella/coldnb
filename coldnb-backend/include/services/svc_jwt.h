#ifndef COLDNB_SVC_JWT_H
#define COLDNB_SVC_JWT_H

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* JWT parsing result */
typedef struct {
    cJSON *header;
    cJSON *payload;
    char *signature;
    bool valid;
    char *error;
} JwtToken;

/* Parse a JWT token (does not verify signature)
 * Returns newly allocated JwtToken (caller must free with jwt_token_free)
 * Sets valid=false and error message if parsing fails */
JwtToken *jwt_parse(const char *token);

/* Free a JWT token */
void jwt_token_free(JwtToken *jwt);

/* Verify JWT signature using HMAC-SHA256
 * Returns true if signature is valid */
bool jwt_verify_hs256(const char *token, const char *secret);

/* Get claim from JWT payload */
const char *jwt_get_string(const JwtToken *jwt, const char *claim);
int64_t jwt_get_int(const JwtToken *jwt, const char *claim);
double jwt_get_double(const JwtToken *jwt, const char *claim);
bool jwt_get_bool(const JwtToken *jwt, const char *claim);
cJSON *jwt_get_object(const JwtToken *jwt, const char *claim);
cJSON *jwt_get_array(const JwtToken *jwt, const char *claim);

/* Check if JWT is expired */
bool jwt_is_expired(const JwtToken *jwt);

/* Get expiration time */
time_t jwt_get_exp(const JwtToken *jwt);

/* Get issued at time */
time_t jwt_get_iat(const JwtToken *jwt);

/* Get subject (sub claim) */
const char *jwt_get_sub(const JwtToken *jwt);

/* Get issuer (iss claim) */
const char *jwt_get_iss(const JwtToken *jwt);

/* Get audience (aud claim) */
const char *jwt_get_aud(const JwtToken *jwt);

/* Create a new JWT token
 * Returns newly allocated token string (caller must free)
 * exp_seconds is number of seconds from now until expiration */
char *jwt_create_hs256(const cJSON *payload, const char *secret, int exp_seconds);

/* Add standard claims to payload */
void jwt_add_standard_claims(cJSON *payload, const char *issuer,
                             const char *subject, int exp_seconds);

#endif /* COLDNB_SVC_JWT_H */
