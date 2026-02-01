#ifndef COLDNB_STRING_UTIL_H
#define COLDNB_STRING_UTIL_H

#include <stdbool.h>
#include <stddef.h>

/* Safe string duplication
 * Returns NULL on allocation failure or if src is NULL */
char *str_dup(const char *src);

/* Safe string duplication with length limit
 * Returns NULL on allocation failure or if src is NULL */
char *str_ndup(const char *src, size_t n);

/* Trim whitespace from both ends (modifies string in place)
 * Returns pointer to trimmed string (may be offset from original) */
char *str_trim(char *str);

/* Trim whitespace and return new allocated string
 * Caller must free */
char *str_trim_dup(const char *str);

/* Check if string starts with prefix */
bool str_starts_with(const char *str, const char *prefix);

/* Check if string ends with suffix */
bool str_ends_with(const char *str, const char *suffix);

/* Case-insensitive string comparison */
int str_casecmp(const char *s1, const char *s2);

/* Case-insensitive string comparison with length limit */
int str_ncasecmp(const char *s1, const char *s2, size_t n);

/* URL decode a string
 * Returns new allocated string, caller must free
 * Returns NULL on allocation failure */
char *str_url_decode(const char *src);

/* URL encode a string
 * Returns new allocated string, caller must free
 * Returns NULL on allocation failure */
char *str_url_encode(const char *src);

/* Split string by delimiter
 * Returns array of strings, terminated by NULL
 * Caller must free each string and the array itself
 * max_parts limits number of splits (0 = unlimited) */
char **str_split(const char *str, char delimiter, size_t max_parts);

/* Free a string array returned by str_split */
void str_split_free(char **parts);

/* Join strings with delimiter
 * Returns new allocated string, caller must free */
char *str_join(const char **parts, const char *delimiter);

/* Safe string concatenation
 * Returns new allocated string containing s1 + s2
 * Caller must free */
char *str_concat(const char *s1, const char *s2);

/* Safe snprintf that returns allocated string
 * Caller must free */
char *str_printf(const char *fmt, ...);

/* Check if string is empty (NULL or zero length) */
bool str_is_empty(const char *str);

/* Check if string contains only whitespace */
bool str_is_blank(const char *str);

/* Convert string to lowercase (modifies in place)
 * Returns the input pointer */
char *str_to_lower(char *str);

/* Convert string to uppercase (modifies in place)
 * Returns the input pointer */
char *str_to_upper(char *str);

/* Replace all occurrences of 'old' with 'new'
 * Returns new allocated string, caller must free */
char *str_replace(const char *str, const char *old_str, const char *new_str);

/* Escape HTML special characters
 * Returns new allocated string, caller must free */
char *str_escape_html(const char *str);

/* Generate a random alphanumeric string
 * Returns new allocated string of specified length, caller must free */
char *str_random(size_t length);

#endif /* COLDNB_STRING_UTIL_H */
