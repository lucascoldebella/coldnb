#include "config/config_loader.h"
#include "log/log.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum line length in config file */
#define MAX_LINE_LENGTH 4096
#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 4096

/* Config entry */
typedef struct ConfigEntry {
    char *key;
    char *value;
    struct ConfigEntry *next;
} ConfigEntry;

/* Config structure */
struct Config {
    char *path;
    ConfigEntry *entries;
    size_t count;
};

/* Trim whitespace from both ends of string (modifies in place) */
static char *trim(char *str) {
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

/* Duplicate a string */
static char *strdup_safe(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup == NULL) {
        return NULL;
    }
    memcpy(dup, str, len + 1);
    return dup;
}

/* Free a config entry */
static void entry_free(ConfigEntry *entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->key);
    free(entry->value);
    free(entry);
}

/* Find entry by key */
static ConfigEntry *find_entry(const Config *config, const char *key) {
    if (config == NULL || key == NULL) {
        return NULL;
    }

    ConfigEntry *entry = config->entries;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Add or update entry */
static int set_entry(Config *config, const char *key, const char *value) {
    if (config == NULL || key == NULL) {
        return -1;
    }

    /* Check for existing entry */
    ConfigEntry *existing = find_entry(config, key);
    if (existing != NULL) {
        /* Update existing entry */
        char *new_value = strdup_safe(value);
        if (new_value == NULL && value != NULL) {
            return -1;
        }
        free(existing->value);
        existing->value = new_value;
        return 0;
    }

    /* Create new entry */
    ConfigEntry *entry = calloc(1, sizeof(ConfigEntry));
    if (entry == NULL) {
        return -1;
    }

    entry->key = strdup_safe(key);
    entry->value = strdup_safe(value);

    if (entry->key == NULL || (entry->value == NULL && value != NULL)) {
        entry_free(entry);
        return -1;
    }

    /* Add to front of list */
    entry->next = config->entries;
    config->entries = entry;
    config->count++;

    return 0;
}

/* Parse a single line */
static int parse_line(Config *config, const char *line, int line_num) {
    char buffer[MAX_LINE_LENGTH];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *trimmed = trim(buffer);

    /* Skip empty lines and comments */
    if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
        return 0;
    }

    /* Find equals sign */
    char *equals = strchr(trimmed, '=');
    if (equals == NULL) {
        LOG_WARN("Config line %d: missing '=' in '%s'", line_num, trimmed);
        return 0;  /* Non-fatal, skip line */
    }

    /* Split key and value */
    *equals = '\0';
    char *key = trim(trimmed);
    char *value = trim(equals + 1);

    /* Validate key */
    if (strlen(key) == 0) {
        LOG_WARN("Config line %d: empty key", line_num);
        return 0;
    }

    if (strlen(key) >= MAX_KEY_LENGTH) {
        LOG_WARN("Config line %d: key too long", line_num);
        return 0;
    }

    /* Add entry */
    if (set_entry(config, key, value) != 0) {
        LOG_ERROR("Config line %d: failed to add entry", line_num);
        return -1;
    }

    return 0;
}

Config *config_load(const char *path) {
    if (path == NULL) {
        LOG_ERROR("config_load: path is NULL");
        return NULL;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        LOG_ERROR("Failed to open config file '%s': %s", path, strerror(errno));
        return NULL;
    }

    Config *config = calloc(1, sizeof(Config));
    if (config == NULL) {
        LOG_ERROR("Failed to allocate config structure");
        fclose(file);
        return NULL;
    }

    config->path = strdup_safe(path);
    if (config->path == NULL) {
        LOG_ERROR("Failed to duplicate config path");
        free(config);
        fclose(file);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_num++;
        if (parse_line(config, line, line_num) != 0) {
            LOG_ERROR("Failed to parse config at line %d", line_num);
            config_free(config);
            fclose(file);
            return NULL;
        }
    }

    if (ferror(file)) {
        LOG_ERROR("Error reading config file: %s", strerror(errno));
        config_free(config);
        fclose(file);
        return NULL;
    }

    fclose(file);
    LOG_INFO("Loaded config from '%s' (%zu entries)", path, config->count);
    return config;
}

void config_free(Config *config) {
    if (config == NULL) {
        return;
    }

    ConfigEntry *entry = config->entries;
    while (entry != NULL) {
        ConfigEntry *next = entry->next;
        entry_free(entry);
        entry = next;
    }

    free(config->path);
    free(config);
}

const char *config_get_string(const Config *config, const char *key,
                              const char *default_val) {
    ConfigEntry *entry = find_entry(config, key);
    if (entry == NULL || entry->value == NULL) {
        return default_val;
    }
    return entry->value;
}

int config_get_int(const Config *config, const char *key, int default_val) {
    const char *value = config_get_string(config, key, NULL);
    if (value == NULL) {
        return default_val;
    }

    char *endptr;
    errno = 0;
    long result = strtol(value, &endptr, 10);

    if (errno != 0 || endptr == value || *endptr != '\0') {
        LOG_WARN("Config key '%s': invalid integer '%s'", key, value);
        return default_val;
    }

    if (result < INT32_MIN || result > INT32_MAX) {
        LOG_WARN("Config key '%s': value out of int range", key);
        return default_val;
    }

    return (int)result;
}

long config_get_long(const Config *config, const char *key, long default_val) {
    const char *value = config_get_string(config, key, NULL);
    if (value == NULL) {
        return default_val;
    }

    char *endptr;
    errno = 0;
    long result = strtol(value, &endptr, 10);

    if (errno != 0 || endptr == value || *endptr != '\0') {
        LOG_WARN("Config key '%s': invalid long '%s'", key, value);
        return default_val;
    }

    return result;
}

size_t config_get_size(const Config *config, const char *key, size_t default_val) {
    const char *value = config_get_string(config, key, NULL);
    if (value == NULL) {
        return default_val;
    }

    char *endptr;
    errno = 0;
    unsigned long long result = strtoull(value, &endptr, 10);

    if (errno != 0 || endptr == value || *endptr != '\0') {
        LOG_WARN("Config key '%s': invalid size '%s'", key, value);
        return default_val;
    }

    return (size_t)result;
}

bool config_get_bool(const Config *config, const char *key, bool default_val) {
    const char *value = config_get_string(config, key, NULL);
    if (value == NULL) {
        return default_val;
    }

    /* True values */
    if (strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "on") == 0 ||
        strcmp(value, "1") == 0) {
        return true;
    }

    /* False values */
    if (strcasecmp(value, "false") == 0 ||
        strcasecmp(value, "no") == 0 ||
        strcasecmp(value, "off") == 0 ||
        strcmp(value, "0") == 0) {
        return false;
    }

    LOG_WARN("Config key '%s': invalid boolean '%s'", key, value);
    return default_val;
}

double config_get_double(const Config *config, const char *key, double default_val) {
    const char *value = config_get_string(config, key, NULL);
    if (value == NULL) {
        return default_val;
    }

    char *endptr;
    errno = 0;
    double result = strtod(value, &endptr);

    if (errno != 0 || endptr == value || *endptr != '\0') {
        LOG_WARN("Config key '%s': invalid double '%s'", key, value);
        return default_val;
    }

    return result;
}

char *config_load_secret(const Config *config, const char *key) {
    const char *file_path = config_get_string(config, key, NULL);
    if (file_path == NULL || file_path[0] == '\0') {
        LOG_ERROR("Secret key '%s' not found in config", key);
        return NULL;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        LOG_ERROR("Failed to open secret file '%s': %s", file_path, strerror(errno));
        return NULL;
    }

    /* Read entire file */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0 || file_size > MAX_VALUE_LENGTH) {
        LOG_ERROR("Secret file '%s' has invalid size", file_path);
        fclose(file);
        return NULL;
    }

    char *secret = malloc((size_t)file_size + 1);
    if (secret == NULL) {
        LOG_ERROR("Failed to allocate memory for secret");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(secret, 1, (size_t)file_size, file);
    fclose(file);

    if (read_size == 0) {
        LOG_ERROR("Failed to read secret file '%s'", file_path);
        free(secret);
        return NULL;
    }

    secret[read_size] = '\0';

    /* Trim whitespace */
    char *trimmed = trim(secret);
    if (trimmed != secret) {
        memmove(secret, trimmed, strlen(trimmed) + 1);
    }

    /* Remove trailing newlines specifically */
    size_t len = strlen(secret);
    while (len > 0 && (secret[len - 1] == '\n' || secret[len - 1] == '\r')) {
        secret[--len] = '\0';
    }

    LOG_DEBUG("Loaded secret from '%s'", file_path);
    return secret;
}

bool config_has_key(const Config *config, const char *key) {
    return find_entry(config, key) != NULL;
}

void config_iterate(const Config *config, ConfigIterator callback, void *user_data) {
    if (config == NULL || callback == NULL) {
        return;
    }

    ConfigEntry *entry = config->entries;
    while (entry != NULL) {
        if (!callback(entry->key, entry->value, user_data)) {
            break;
        }
        entry = entry->next;
    }
}

int config_reload(Config *config) {
    if (config == NULL || config->path == NULL) {
        return -1;
    }

    /* Load new config */
    Config *new_config = config_load(config->path);
    if (new_config == NULL) {
        LOG_ERROR("Failed to reload config");
        return -1;
    }

    /* Swap entries */
    ConfigEntry *old_entries = config->entries;
    config->entries = new_config->entries;
    config->count = new_config->count;

    /* Free old entries */
    new_config->entries = old_entries;
    config_free(new_config);

    LOG_INFO("Config reloaded from '%s'", config->path);
    return 0;
}

const char *config_get_path(const Config *config) {
    if (config == NULL) {
        return NULL;
    }
    return config->path;
}
