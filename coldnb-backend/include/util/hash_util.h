#ifndef COLDNB_HASH_UTIL_H
#define COLDNB_HASH_UTIL_H

#include <stdbool.h>
#include <stddef.h>

/* Initialize the hashing library (call once at startup) */
int hash_init(void);

/* Hash a password using Argon2id
 * Returns newly allocated hash string (caller must free) or NULL on error
 * The returned string includes the algorithm parameters and salt */
char *hash_password(const char *password);

/* Verify a password against a hash
 * Returns true if password matches, false otherwise */
bool hash_password_verify(const char *password, const char *hash);

/* Generate a secure random hex string
 * Returns newly allocated string (caller must free) or NULL on error
 * length is the number of random bytes (output will be 2*length chars) */
char *hash_random_hex(size_t length);

/* Generate a secure random base64 string
 * Returns newly allocated string (caller must free) or NULL on error */
char *hash_random_base64(size_t length);

/* Calculate SHA256 hash of data
 * Returns newly allocated hex string (64 chars + null) or NULL on error */
char *hash_sha256(const void *data, size_t size);

/* Calculate SHA256 hash of string */
char *hash_sha256_string(const char *str);

/* Calculate HMAC-SHA256
 * Returns newly allocated hex string or NULL on error */
char *hash_hmac_sha256(const void *key, size_t key_len,
                       const void *data, size_t data_len);

/* Base64 encode data
 * Returns newly allocated string or NULL on error */
char *hash_base64_encode(const void *data, size_t size);

/* Base64 decode string
 * Returns newly allocated buffer (caller must free) and sets output_size
 * Returns NULL on error */
void *hash_base64_decode(const char *str, size_t *output_size);

/* URL-safe base64 encode (replaces + with -, / with _, removes padding) */
char *hash_base64url_encode(const void *data, size_t size);

/* URL-safe base64 decode */
void *hash_base64url_decode(const char *str, size_t *output_size);

/* Constant-time string comparison (prevents timing attacks) */
bool hash_constant_compare(const char *a, const char *b);

#endif /* COLDNB_HASH_UTIL_H */
