#include "util/json_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

const char *json_get_string(const cJSON *obj, const char *key, const char *default_val) {
    if (obj == NULL || key == NULL) {
        return default_val;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsString(item) || item->valuestring == NULL) {
        return default_val;
    }

    return item->valuestring;
}

int json_get_int(const cJSON *obj, const char *key, int default_val) {
    if (obj == NULL || key == NULL) {
        return default_val;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return default_val;
    }

    return item->valueint;
}

int64_t json_get_int64(const cJSON *obj, const char *key, int64_t default_val) {
    if (obj == NULL || key == NULL) {
        return default_val;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return default_val;
    }

    return (int64_t)item->valuedouble;
}

double json_get_double(const cJSON *obj, const char *key, double default_val) {
    if (obj == NULL || key == NULL) {
        return default_val;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsNumber(item)) {
        return default_val;
    }

    return item->valuedouble;
}

bool json_get_bool(const cJSON *obj, const char *key, bool default_val) {
    if (obj == NULL || key == NULL) {
        return default_val;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL) {
        return default_val;
    }

    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }

    return default_val;
}

cJSON *json_get_array(const cJSON *obj, const char *key) {
    if (obj == NULL || key == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsArray(item)) {
        return NULL;
    }

    return item;
}

cJSON *json_get_object(const cJSON *obj, const char *key) {
    if (obj == NULL || key == NULL) {
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL || !cJSON_IsObject(item)) {
        return NULL;
    }

    return item;
}

bool json_has_key(const cJSON *obj, const char *key) {
    if (obj == NULL || key == NULL) {
        return false;
    }
    return cJSON_HasObjectItem(obj, key);
}

bool json_is_null(const cJSON *obj, const char *key) {
    if (obj == NULL || key == NULL) {
        return true;
    }

    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return item == NULL || cJSON_IsNull(item);
}

cJSON *json_create_response(bool success, const char *message) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddBoolToObject(obj, "success", success);
    if (message != NULL) {
        cJSON_AddStringToObject(obj, "message", message);
    }

    return obj;
}

cJSON *json_create_error(int status, const char *message) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON *error = cJSON_CreateObject();
    if (error == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }

    cJSON_AddNumberToObject(error, "status", status);
    cJSON_AddStringToObject(error, "message", message ? message : "Error");
    cJSON_AddItemToObject(obj, "error", error);

    return obj;
}

cJSON *json_create_success(cJSON *data) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddBoolToObject(obj, "success", true);
    if (data != NULL) {
        cJSON_AddItemToObject(obj, "data", data);
    }

    return obj;
}

cJSON *json_create_pagination(int page, int per_page, int total, int total_pages) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "page", page);
    cJSON_AddNumberToObject(obj, "per_page", per_page);
    cJSON_AddNumberToObject(obj, "total", total);
    cJSON_AddNumberToObject(obj, "total_pages", total_pages);

    return obj;
}

char *json_to_string(const cJSON *obj) {
    if (obj == NULL) {
        return NULL;
    }
    return cJSON_PrintUnformatted(obj);
}

cJSON *json_parse(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    cJSON *obj = cJSON_Parse(str);
    if (obj == NULL) {
        const char *error = cJSON_GetErrorPtr();
        if (error != NULL) {
            LOG_WARN("JSON parse error near: %.50s", error);
        }
    }

    return obj;
}

void json_add_string_if(cJSON *obj, const char *key, const char *value) {
    if (obj == NULL || key == NULL || value == NULL) {
        return;
    }
    cJSON_AddStringToObject(obj, key, value);
}

void json_add_number_if(cJSON *obj, const char *key, double value) {
    if (obj == NULL || key == NULL || value == 0.0) {
        return;
    }
    cJSON_AddNumberToObject(obj, key, value);
}

cJSON *json_deep_copy(const cJSON *obj) {
    if (obj == NULL) {
        return NULL;
    }
    return cJSON_Duplicate(obj, true);
}

void json_merge(cJSON *dest, const cJSON *src) {
    if (dest == NULL || src == NULL) {
        return;
    }

    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, src) {
        cJSON *copy = cJSON_Duplicate(item, true);
        if (copy != NULL) {
            cJSON_DeleteItemFromObject(dest, item->string);
            cJSON_AddItemToObject(dest, item->string, copy);
        }
    }
}
