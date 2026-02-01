#ifndef COLDNB_UUID_UTIL_H
#define COLDNB_UUID_UTIL_H

#include <stdbool.h>

/* UUID string length (36 chars + null) */
#define UUID_STRING_LENGTH 37

/* Generate a new UUID v4 (random)
 * Returns newly allocated string (caller must free) or NULL on error */
char *uuid_generate(void);

/* Generate a new UUID v4 into provided buffer
 * Buffer must be at least UUID_STRING_LENGTH bytes
 * Returns 0 on success, -1 on error */
int uuid_generate_buf(char *buffer, size_t size);

/* Validate UUID format
 * Returns true if string is a valid UUID format */
bool uuid_validate(const char *str);

/* Compare two UUIDs (case-insensitive)
 * Returns 0 if equal, non-zero otherwise */
int uuid_compare(const char *a, const char *b);

/* Convert UUID to lowercase
 * Modifies string in place, returns input pointer */
char *uuid_to_lower(char *uuid);

/* Convert UUID to uppercase
 * Modifies string in place, returns input pointer */
char *uuid_to_upper(char *uuid);

#endif /* COLDNB_UUID_UTIL_H */
