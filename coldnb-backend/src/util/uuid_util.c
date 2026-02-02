#include "util/uuid_util.h"
#include "util/string_util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

char *coldnb_uuid_generate(void) {
    char *buffer = malloc(UUID_STRING_LENGTH);
    if (buffer == NULL) {
        return NULL;
    }

    if (coldnb_uuid_generate_buf(buffer, UUID_STRING_LENGTH) != 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

int coldnb_uuid_generate_buf(char *buffer, size_t size) {
    if (buffer == NULL || size < UUID_STRING_LENGTH) {
        return -1;
    }

    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, buffer);

    return 0;
}

bool uuid_validate(const char *str) {
    if (str == NULL) {
        return false;
    }

    /* UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    /* Length: 36 characters */
    size_t len = strlen(str);
    if (len != 36) {
        return false;
    }

    /* Check dashes */
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return false;
    }

    /* Check hex characters */
    for (size_t i = 0; i < len; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            continue;  /* Skip dashes */
        }
        if (!isxdigit((unsigned char)str[i])) {
            return false;
        }
    }

    return true;
}

int coldnb_uuid_compare(const char *a, const char *b) {
    if (a == NULL && b == NULL) {
        return 0;
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    return strcasecmp(a, b);
}

char *uuid_to_lower(char *uuid) {
    return str_to_lower(uuid);
}

char *uuid_to_upper(char *uuid) {
    return str_to_upper(uuid);
}
