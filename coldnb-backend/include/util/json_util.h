#ifndef COLDNB_JSON_UTIL_H
#define COLDNB_JSON_UTIL_H

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>

/* Get string from JSON object, returns default_val if not found or not string */
const char *json_get_string(const cJSON *obj, const char *key, const char *default_val);

/* Get integer from JSON object, returns default_val if not found or not number */
int json_get_int(const cJSON *obj, const char *key, int default_val);

/* Get int64 from JSON object */
int64_t json_get_int64(const cJSON *obj, const char *key, int64_t default_val);

/* Get double from JSON object */
double json_get_double(const cJSON *obj, const char *key, double default_val);

/* Get boolean from JSON object */
bool json_get_bool(const cJSON *obj, const char *key, bool default_val);

/* Get array from JSON object, returns NULL if not found or not array */
cJSON *json_get_array(const cJSON *obj, const char *key);

/* Get object from JSON object, returns NULL if not found or not object */
cJSON *json_get_object(const cJSON *obj, const char *key);

/* Check if key exists in JSON object */
bool json_has_key(const cJSON *obj, const char *key);

/* Check if value is null */
bool json_is_null(const cJSON *obj, const char *key);

/* Create a new JSON object with common fields */
cJSON *json_create_response(bool success, const char *message);

/* Create error response JSON */
cJSON *json_create_error(int status, const char *message);

/* Create success response with data */
cJSON *json_create_success(cJSON *data);

/* Create pagination info object */
cJSON *json_create_pagination(int page, int per_page, int total, int total_pages);

/* Safe JSON string (returns new allocated JSON string) */
char *json_to_string(const cJSON *obj);

/* Parse JSON string with error handling */
cJSON *json_parse(const char *str);

/* Add string to object only if value is not NULL */
void json_add_string_if(cJSON *obj, const char *key, const char *value);

/* Add number to object only if value is not 0 */
void json_add_number_if(cJSON *obj, const char *key, double value);

/* Deep copy a cJSON object */
cJSON *json_deep_copy(const cJSON *obj);

/* Merge two JSON objects (src into dest) */
void json_merge(cJSON *dest, const cJSON *src);

#endif /* COLDNB_JSON_UTIL_H */
