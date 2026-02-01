#include "util/hash_util.h"
#include "log/log.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

static bool initialized = false;

int hash_init(void) {
    if (initialized) {
        return 0;
    }

    if (sodium_init() < 0) {
        LOG_ERROR("Failed to initialize libsodium");
        return -1;
    }

    initialized = true;
    return 0;
}

char *hash_password(const char *password) {
    if (password == NULL) {
        return NULL;
    }

    if (!initialized && hash_init() != 0) {
        return NULL;
    }

    /* Allocate buffer for hash (includes algorithm params and salt) */
    char *hash = malloc(crypto_pwhash_STRBYTES);
    if (hash == NULL) {
        return NULL;
    }

    /* Use Argon2id with moderate parameters */
    if (crypto_pwhash_str(
            hash,
            password,
            strlen(password),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        LOG_ERROR("Password hashing failed");
        free(hash);
        return NULL;
    }

    return hash;
}

bool hash_password_verify(const char *password, const char *hash) {
    if (password == NULL || hash == NULL) {
        return false;
    }

    if (!initialized && hash_init() != 0) {
        return false;
    }

    return crypto_pwhash_str_verify(hash, password, strlen(password)) == 0;
}

char *hash_random_hex(size_t length) {
    if (length == 0) {
        return NULL;
    }

    unsigned char *bytes = malloc(length);
    if (bytes == NULL) {
        return NULL;
    }

    /* Generate random bytes */
    if (RAND_bytes(bytes, (int)length) != 1) {
        free(bytes);
        return NULL;
    }

    /* Convert to hex */
    char *hex = malloc(length * 2 + 1);
    if (hex == NULL) {
        free(bytes);
        return NULL;
    }

    for (size_t i = 0; i < length; i++) {
        snprintf(hex + i * 2, 3, "%02x", bytes[i]);
    }
    hex[length * 2] = '\0';

    free(bytes);
    return hex;
}

char *hash_random_base64(size_t length) {
    if (length == 0) {
        return NULL;
    }

    unsigned char *bytes = malloc(length);
    if (bytes == NULL) {
        return NULL;
    }

    if (RAND_bytes(bytes, (int)length) != 1) {
        free(bytes);
        return NULL;
    }

    char *result = hash_base64_encode(bytes, length);
    free(bytes);
    return result;
}

char *hash_sha256(const void *data, size_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        return NULL;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, data, size) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return NULL;
    }

    EVP_MD_CTX_free(ctx);

    /* Convert to hex */
    char *hex = malloc(hash_len * 2 + 1);
    if (hex == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < hash_len; i++) {
        snprintf(hex + i * 2, 3, "%02x", hash[i]);
    }
    hex[hash_len * 2] = '\0';

    return hex;
}

char *hash_sha256_string(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    return hash_sha256(str, strlen(str));
}

char *hash_hmac_sha256(const void *key, size_t key_len,
                       const void *data, size_t data_len) {
    if (key == NULL || data == NULL) {
        return NULL;
    }

    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len;

    if (HMAC(EVP_sha256(), key, (int)key_len, data, data_len,
             result, &result_len) == NULL) {
        return NULL;
    }

    /* Convert to hex */
    char *hex = malloc(result_len * 2 + 1);
    if (hex == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < result_len; i++) {
        snprintf(hex + i * 2, 3, "%02x", result[i]);
    }
    hex[result_len * 2] = '\0';

    return hex;
}

char *hash_base64_encode(const void *data, size_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }

    /* Calculate output size */
    size_t output_size = ((size + 2) / 3) * 4 + 1;
    char *output = malloc(output_size);
    if (output == NULL) {
        return NULL;
    }

    int len = EVP_EncodeBlock((unsigned char *)output, data, (int)size);
    output[len] = '\0';

    return output;
}

void *hash_base64_decode(const char *str, size_t *output_size) {
    if (str == NULL || output_size == NULL) {
        return NULL;
    }

    size_t input_len = strlen(str);
    if (input_len == 0) {
        *output_size = 0;
        return NULL;
    }

    /* Calculate maximum output size */
    size_t max_output = (input_len / 4) * 3;
    unsigned char *output = malloc(max_output + 1);
    if (output == NULL) {
        return NULL;
    }

    int len = EVP_DecodeBlock(output, (const unsigned char *)str, (int)input_len);
    if (len < 0) {
        free(output);
        return NULL;
    }

    /* Adjust for padding */
    if (input_len > 0 && str[input_len - 1] == '=') {
        len--;
    }
    if (input_len > 1 && str[input_len - 2] == '=') {
        len--;
    }

    *output_size = (size_t)len;
    return output;
}

char *hash_base64url_encode(const void *data, size_t size) {
    char *base64 = hash_base64_encode(data, size);
    if (base64 == NULL) {
        return NULL;
    }

    /* Replace + with -, / with _ */
    for (char *p = base64; *p; p++) {
        if (*p == '+') {
            *p = '-';
        } else if (*p == '/') {
            *p = '_';
        }
    }

    /* Remove padding */
    size_t len = strlen(base64);
    while (len > 0 && base64[len - 1] == '=') {
        base64[--len] = '\0';
    }

    return base64;
}

void *hash_base64url_decode(const char *str, size_t *output_size) {
    if (str == NULL || output_size == NULL) {
        return NULL;
    }

    size_t len = strlen(str);

    /* Add padding if needed */
    size_t padded_len = len + (4 - len % 4) % 4;
    char *padded = malloc(padded_len + 1);
    if (padded == NULL) {
        return NULL;
    }

    /* Copy and replace URL-safe chars */
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '-') {
            padded[i] = '+';
        } else if (str[i] == '_') {
            padded[i] = '/';
        } else {
            padded[i] = str[i];
        }
    }

    /* Add padding */
    for (size_t i = len; i < padded_len; i++) {
        padded[i] = '=';
    }
    padded[padded_len] = '\0';

    void *result = hash_base64_decode(padded, output_size);
    free(padded);
    return result;
}

bool hash_constant_compare(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    /* Use constant-time comparison */
    if (len_a != len_b) {
        return false;
    }

    return sodium_memcmp(a, b, len_a) == 0;
}
