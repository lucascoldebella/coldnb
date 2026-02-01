#ifndef COLDNB_CONFIG_LOADER_H
#define COLDNB_CONFIG_LOADER_H

#include <stdbool.h>
#include <stddef.h>

/* Opaque config handle */
typedef struct Config Config;

/* Load configuration from file
 * Returns NULL on error */
Config *config_load(const char *path);

/* Free configuration */
void config_free(Config *config);

/* Get string value
 * Returns default_val if key not found */
const char *config_get_string(const Config *config, const char *key,
                              const char *default_val);

/* Get integer value
 * Returns default_val if key not found or invalid */
int config_get_int(const Config *config, const char *key, int default_val);

/* Get long value
 * Returns default_val if key not found or invalid */
long config_get_long(const Config *config, const char *key, long default_val);

/* Get size_t value
 * Returns default_val if key not found or invalid */
size_t config_get_size(const Config *config, const char *key, size_t default_val);

/* Get boolean value (true/false, yes/no, 1/0)
 * Returns default_val if key not found or invalid */
bool config_get_bool(const Config *config, const char *key, bool default_val);

/* Get double value
 * Returns default_val if key not found or invalid */
double config_get_double(const Config *config, const char *key, double default_val);

/* Load secret from file specified in config
 * Key should point to a config value containing a file path
 * Returns newly allocated string (caller must free) or NULL on error
 * Trims whitespace from the loaded secret */
char *config_load_secret(const Config *config, const char *key);

/* Check if key exists in config */
bool config_has_key(const Config *config, const char *key);

/* Iterate over all config entries
 * Callback receives key, value, and user data
 * Return false from callback to stop iteration */
typedef bool (*ConfigIterator)(const char *key, const char *value, void *user_data);
void config_iterate(const Config *config, ConfigIterator callback, void *user_data);

/* Reload configuration from file
 * Returns 0 on success, -1 on error
 * On error, existing config is preserved */
int config_reload(Config *config);

/* Get the path of the loaded config file */
const char *config_get_path(const Config *config);

#endif /* COLDNB_CONFIG_LOADER_H */
