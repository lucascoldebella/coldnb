#include "util/string_util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *str_dup(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    char *dst = malloc(len + 1);
    if (dst == NULL) {
        return NULL;
    }
    memcpy(dst, src, len + 1);
    return dst;
}

char *str_ndup(const char *src, size_t n) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    if (n < len) {
        len = n;
    }
    char *dst = malloc(len + 1);
    if (dst == NULL) {
        return NULL;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

char *str_trim(char *str) {
    if (str == NULL) {
        return NULL;
    }

    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';

    return str;
}

char *str_trim_dup(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    /* Find start */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str_dup("");
    }

    /* Find end */
    const char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    return str_ndup(str, (size_t)(end - str + 1));
}

bool str_starts_with(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) {
        return false;
    }
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

bool str_ends_with(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return false;
    }
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int str_casecmp(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return strcasecmp(s1, s2);
}

int str_ncasecmp(const char *s1, const char *s2, size_t n) {
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return strncasecmp(s1, s2, n);
}

/* Convert hex character to value */
static int hex_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

char *str_url_decode(const char *src) {
    if (src == NULL) {
        return NULL;
    }

    size_t src_len = strlen(src);
    char *dst = malloc(src_len + 1);
    if (dst == NULL) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int high = hex_to_int(src[i + 1]);
            int low = hex_to_int(src[i + 2]);
            if (high >= 0 && low >= 0) {
                dst[j++] = (char)((high << 4) | low);
                i += 2;
                continue;
            }
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            continue;
        }
        dst[j++] = src[i];
    }
    dst[j] = '\0';

    return dst;
}

char *str_url_encode(const char *src) {
    if (src == NULL) {
        return NULL;
    }

    static const char *hex = "0123456789ABCDEF";
    size_t src_len = strlen(src);

    /* Worst case: every character needs encoding */
    char *dst = malloc(src_len * 3 + 1);
    if (dst == NULL) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < src_len; i++) {
        unsigned char c = (unsigned char)src[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[j++] = (char)c;
        } else if (c == ' ') {
            dst[j++] = '+';
        } else {
            dst[j++] = '%';
            dst[j++] = hex[(c >> 4) & 0x0F];
            dst[j++] = hex[c & 0x0F];
        }
    }
    dst[j] = '\0';

    return dst;
}

char **str_split(const char *str, char delimiter, size_t max_parts) {
    if (str == NULL) {
        return NULL;
    }

    /* Count parts */
    size_t count = 1;
    for (const char *p = str; *p; p++) {
        if (*p == delimiter) {
            count++;
            if (max_parts > 0 && count >= max_parts) {
                break;
            }
        }
    }

    /* Allocate array */
    char **parts = calloc(count + 1, sizeof(char *));
    if (parts == NULL) {
        return NULL;
    }

    /* Split string */
    size_t part_idx = 0;
    const char *start = str;

    for (const char *p = str;; p++) {
        if (*p == delimiter || *p == '\0') {
            if (max_parts > 0 && part_idx >= max_parts - 1) {
                /* Last part gets the rest */
                parts[part_idx] = str_dup(start);
                break;
            }

            parts[part_idx] = str_ndup(start, (size_t)(p - start));
            if (parts[part_idx] == NULL) {
                str_split_free(parts);
                return NULL;
            }
            part_idx++;

            if (*p == '\0') {
                break;
            }
            start = p + 1;
        }
    }

    return parts;
}

void str_split_free(char **parts) {
    if (parts == NULL) {
        return;
    }
    for (size_t i = 0; parts[i] != NULL; i++) {
        free(parts[i]);
    }
    free(parts);
}

char *str_join(const char **parts, const char *delimiter) {
    if (parts == NULL) {
        return NULL;
    }
    if (parts[0] == NULL) {
        return str_dup("");
    }

    /* Calculate total length */
    size_t delim_len = delimiter ? strlen(delimiter) : 0;
    size_t total_len = 0;
    size_t count = 0;

    for (size_t i = 0; parts[i] != NULL; i++) {
        total_len += strlen(parts[i]);
        count++;
    }
    total_len += delim_len * (count - 1);

    /* Allocate result */
    char *result = malloc(total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    /* Join */
    char *p = result;
    for (size_t i = 0; parts[i] != NULL; i++) {
        if (i > 0 && delimiter) {
            memcpy(p, delimiter, delim_len);
            p += delim_len;
        }
        size_t len = strlen(parts[i]);
        memcpy(p, parts[i], len);
        p += len;
    }
    *p = '\0';

    return result;
}

char *str_concat(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL) {
        return NULL;
    }
    if (s1 == NULL) {
        return str_dup(s2);
    }
    if (s2 == NULL) {
        return str_dup(s1);
    }

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    char *result = malloc(len1 + len2 + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);

    return result;
}

char *str_printf(const char *fmt, ...) {
    if (fmt == NULL) {
        return NULL;
    }

    va_list args;

    /* First pass: determine required size */
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (size < 0) {
        return NULL;
    }

    /* Allocate buffer */
    char *result = malloc((size_t)size + 1);
    if (result == NULL) {
        return NULL;
    }

    /* Second pass: format string */
    va_start(args, fmt);
    vsnprintf(result, (size_t)size + 1, fmt, args);
    va_end(args);

    return result;
}

bool str_is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

bool str_is_blank(const char *str) {
    if (str == NULL) {
        return true;
    }
    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}

char *str_to_lower(char *str) {
    if (str == NULL) {
        return NULL;
    }
    for (char *p = str; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
    return str;
}

char *str_to_upper(char *str) {
    if (str == NULL) {
        return NULL;
    }
    for (char *p = str; *p; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return str;
}

char *str_replace(const char *str, const char *old_str, const char *new_str) {
    if (str == NULL || old_str == NULL || old_str[0] == '\0') {
        return str_dup(str);
    }
    if (new_str == NULL) {
        new_str = "";
    }

    size_t old_len = strlen(old_str);
    size_t new_len = strlen(new_str);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old_str)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) {
        return str_dup(str);
    }

    /* Calculate result size */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    char *result = malloc(result_len + 1);
    if (result == NULL) {
        return NULL;
    }

    /* Replace */
    char *dst = result;
    p = str;
    while (*p) {
        if (strncmp(p, old_str, old_len) == 0) {
            memcpy(dst, new_str, new_len);
            dst += new_len;
            p += old_len;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';

    return result;
}

char *str_escape_html(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    /* Count special characters */
    size_t extra = 0;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&': extra += 4; break;  /* &amp; */
            case '<': extra += 3; break;  /* &lt; */
            case '>': extra += 3; break;  /* &gt; */
            case '"': extra += 5; break;  /* &quot; */
            case '\'': extra += 5; break; /* &#39; */
            default: break;
        }
    }

    if (extra == 0) {
        return str_dup(str);
    }

    char *result = malloc(strlen(str) + extra + 1);
    if (result == NULL) {
        return NULL;
    }

    char *dst = result;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&':
                memcpy(dst, "&amp;", 5);
                dst += 5;
                break;
            case '<':
                memcpy(dst, "&lt;", 4);
                dst += 4;
                break;
            case '>':
                memcpy(dst, "&gt;", 4);
                dst += 4;
                break;
            case '"':
                memcpy(dst, "&quot;", 6);
                dst += 6;
                break;
            case '\'':
                memcpy(dst, "&#39;", 5);
                dst += 5;
                break;
            default:
                *dst++ = *p;
                break;
        }
    }
    *dst = '\0';

    return result;
}

char *str_random(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    static bool seeded = false;

    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    char *result = malloc(length + 1);
    if (result == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < length; i++) {
        result[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    result[length] = '\0';

    return result;
}
