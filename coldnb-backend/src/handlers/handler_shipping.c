#include "handlers/handler_shipping.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* Origin coordinates: Campo Grande, MS */
#define ORIGIN_LAT  (-20.4697)
#define ORIGIN_LNG  (-54.6201)
#define EARTH_RADIUS_KM 6371.0

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Haversine formula: calculate distance between two lat/lng points */
static double haversine_distance(double lat1, double lng1, double lat2, double lng2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlng = (lng2 - lng1) * M_PI / 180.0;
    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dlng / 2.0) * sin(dlng / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS_KM * c;
}

/* Validate and clean CEP string: accept "12345-678" or "12345678", output "12345678" */
static bool validate_cep(const char *input, char *clean, size_t clean_size) {
    if (input == NULL || clean_size < 9) {
        return false;
    }

    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j < 8; i++) {
        if (isdigit((unsigned char)input[i])) {
            clean[j++] = input[i];
        } else if (input[i] != '-') {
            return false; /* Invalid character */
        }
    }
    clean[j] = '\0';
    return j == 8;
}

void handler_shipping_register(HttpRouter *router, DbPool *pool) {
    /* Public route */
    ROUTE_GET(router, "/api/shipping/calculate", handler_shipping_calculate, pool);

    /* Admin routes */
    ROUTE_GET(router, "/api/admin/shipping/zones", handler_admin_shipping_list, pool);
    ROUTE_POST(router, "/api/admin/shipping/zones", handler_admin_shipping_create, pool);
    ROUTE_PUT(router, "/api/admin/shipping/zones/:id", handler_admin_shipping_update, pool);
    ROUTE_DELETE(router, "/api/admin/shipping/zones/:id", handler_admin_shipping_delete, pool);
}

/* GET /api/shipping/calculate?cep=XXXXX-XXX */
void handler_shipping_calculate(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *cep_raw = http_request_get_query_param(req, "cep");

    if (cep_raw == NULL || cep_raw[0] == '\0') {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "CEP parameter required");
        return;
    }

    /* Clean and validate CEP */
    char cep[16];
    if (!validate_cep(cep_raw, cep, sizeof(cep))) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid CEP format. Use XXXXX-XXX or XXXXXXXX");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Try to find CEP coordinates by progressively shorter prefixes */
    double lat = 0.0;
    double lng = 0.0;
    const char *city = NULL;
    const char *state = NULL;
    bool found = false;

    /* Try 5-digit, 4-digit, 3-digit prefixes */
    for (int prefix_len = 5; prefix_len >= 3 && !found; prefix_len--) {
        char prefix[6];
        snprintf(prefix, sizeof(prefix), "%.*s", prefix_len, cep);

        /* Pad with zeros to 5 chars for DB lookup */
        char padded[6];
        snprintf(padded, sizeof(padded), "%-5s", prefix);
        for (int i = 0; i < 5; i++) {
            if (padded[i] == ' ') {
                padded[i] = '0';
            }
        }

        const char *params[] = { padded };
        PGresult *result = db_exec_params(conn,
            "SELECT latitude, longitude, city, state FROM cep_coordinates WHERE cep_prefix = $1",
            1, params);

        if (db_result_ok(result) && db_result_has_rows(result)) {
            DbRow row = { .result = result, .row = 0 };
            lat = db_row_get_double(&row, "latitude");
            lng = db_row_get_double(&row, "longitude");
            city = str_dup(db_row_get_string(&row, "city"));
            state = str_dup(db_row_get_string(&row, "state"));
            found = true;
        }
        PQclear(result);
    }

    if (!found) {
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "CEP region not found in our coverage area");
        return;
    }

    /* Calculate distance from origin (Campo Grande, MS) */
    double distance_km = haversine_distance(ORIGIN_LAT, ORIGIN_LNG, lat, lng);
    int distance_int = (int)round(distance_km);

    /* Find matching shipping zone */
    char dist_str[32];
    snprintf(dist_str, sizeof(dist_str), "%d", distance_int);
    const char *zone_params[] = { dist_str, dist_str };

    PGresult *zone_result = db_exec_params(conn,
        "SELECT id, name, price, estimated_days_min, estimated_days_max "
        "FROM shipping_zones "
        "WHERE is_active = true AND min_distance_km <= $1 AND max_distance_km >= $2 "
        "ORDER BY sort_order LIMIT 1",
        2, zone_params);

    cJSON *response_data = cJSON_CreateObject();

    /* Format CEP for display */
    char formatted_cep[16];
    snprintf(formatted_cep, sizeof(formatted_cep), "%.5s-%.3s", cep, cep + 5);
    cJSON_AddStringToObject(response_data, "cep", formatted_cep);

    if (city != NULL) {
        cJSON_AddStringToObject(response_data, "city", city);
    }
    if (state != NULL) {
        cJSON_AddStringToObject(response_data, "state", state);
    }
    cJSON_AddNumberToObject(response_data, "distance_km", distance_int);

    if (db_result_ok(zone_result) && db_result_has_rows(zone_result)) {
        DbRow zone_row = { .result = zone_result, .row = 0 };
        const char *zone_name = db_row_get_string(&zone_row, "name");
        double price = db_row_get_double(&zone_row, "price");
        int days_min = db_row_get_int(&zone_row, "estimated_days_min");
        int days_max = db_row_get_int(&zone_row, "estimated_days_max");

        cJSON_AddStringToObject(response_data, "zone", zone_name ? zone_name : "Standard");
        cJSON_AddNumberToObject(response_data, "price", price);
        cJSON_AddNumberToObject(response_data, "estimated_days_min", days_min);
        cJSON_AddNumberToObject(response_data, "estimated_days_max", days_max);
    } else {
        /* No matching zone — use fallback */
        cJSON_AddStringToObject(response_data, "zone", "Remote");
        cJSON_AddNumberToObject(response_data, "price", 60.00);
        cJSON_AddNumberToObject(response_data, "estimated_days_min", 12);
        cJSON_AddNumberToObject(response_data, "estimated_days_max", 20);
    }

    PQclear(zone_result);

    /* Free duplicated strings */
    free((void *)city);
    free((void *)state);

    db_pool_release(pool, conn);

    cJSON *response = json_create_success(response_data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* GET /api/admin/shipping/zones */
void handler_admin_shipping_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    DbPool *pool = (DbPool *)user_data;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    PGresult *result = db_exec(conn,
        "SELECT id, name, min_distance_km, max_distance_km, price, "
        "estimated_days_min, estimated_days_max, is_active, sort_order, "
        "created_at, updated_at "
        "FROM shipping_zones ORDER BY sort_order, id");

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *zones = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_success(zones);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* POST /api/admin/shipping/zones */
void handler_admin_shipping_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON body");
        return;
    }

    const char *name = json_get_string(body, "name", NULL);
    if (name == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name is required");
        return;
    }

    int min_distance = json_get_int(body, "min_distance_km", -1);
    int max_distance = json_get_int(body, "max_distance_km", -1);
    double price = json_get_double(body, "price", -1.0);

    if (min_distance < 0 || max_distance < 0 || price < 0) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
            "min_distance_km, max_distance_km, and price are required");
        return;
    }

    int days_min = json_get_int(body, "estimated_days_min", 1);
    int days_max = json_get_int(body, "estimated_days_max", 3);
    bool is_active = json_get_bool(body, "is_active", true);
    int sort_order = json_get_int(body, "sort_order", 0);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    char min_str[32], max_str[32], price_str[32], days_min_str[16], days_max_str[16];
    char active_str[8], sort_str[16];
    snprintf(min_str, sizeof(min_str), "%d", min_distance);
    snprintf(max_str, sizeof(max_str), "%d", max_distance);
    snprintf(price_str, sizeof(price_str), "%.2f", price);
    snprintf(days_min_str, sizeof(days_min_str), "%d", days_min);
    snprintf(days_max_str, sizeof(days_max_str), "%d", days_max);
    snprintf(active_str, sizeof(active_str), "%s", is_active ? "true" : "false");
    snprintf(sort_str, sizeof(sort_str), "%d", sort_order);

    const char *params[] = {
        name, min_str, max_str, price_str,
        days_min_str, days_max_str, active_str, sort_str
    };

    int64_t new_id = db_insert_returning_id(conn,
        "INSERT INTO shipping_zones "
        "(name, min_distance_km, max_distance_km, price, "
        "estimated_days_min, estimated_days_max, is_active, sort_order) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
        8, params);

    cJSON_Delete(body);

    if (new_id <= 0) {
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create shipping zone");
        return;
    }

    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", (double)new_id);
    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

/* PUT /api/admin/shipping/zones/:id */
void handler_admin_shipping_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (id_str == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Zone ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON body");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build dynamic UPDATE query */
    char set_clause[512];
    size_t set_len = 0;
    int param_count = 0;
    const char *param_values[16];
    char buffers[16][64];

    if (json_has_key(body, "name")) {
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sname = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = json_get_string(body, "name", "");
        param_count++;
    }
    if (json_has_key(body, "min_distance_km")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%d", json_get_int(body, "min_distance_km", 0));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%smin_distance_km = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "max_distance_km")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%d", json_get_int(body, "max_distance_km", 0));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%smax_distance_km = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "price")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%.2f", json_get_double(body, "price", 0));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sprice = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "estimated_days_min")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%d", json_get_int(body, "estimated_days_min", 1));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sestimated_days_min = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "estimated_days_max")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%d", json_get_int(body, "estimated_days_max", 3));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sestimated_days_max = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "is_active")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%s",
                 json_get_bool(body, "is_active", true) ? "true" : "false");
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%sis_active = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }
    if (json_has_key(body, "sort_order")) {
        snprintf(buffers[param_count], sizeof(buffers[0]), "%d", json_get_int(body, "sort_order", 0));
        set_len += (size_t)snprintf(set_clause + set_len, sizeof(set_clause) - set_len,
                     "%ssort_order = $%d", param_count > 0 ? ", " : "", param_count + 1);
        param_values[param_count] = buffers[param_count];
        param_count++;
    }

    (void)set_len;

    if (param_count == 0) {
        cJSON_Delete(body);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "No fields to update");
        return;
    }

    /* Add updated_at */
    snprintf(set_clause + strlen(set_clause), sizeof(set_clause) - strlen(set_clause),
             ", updated_at = CURRENT_TIMESTAMP");

    /* ID is the last parameter */
    param_values[param_count] = id_str;
    param_count++;

    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE shipping_zones SET %s WHERE id = $%d",
             set_clause, param_count);

    PGresult *result = db_exec_params(conn, query, param_count, param_values);
    cJSON_Delete(body);

    if (!db_result_ok(result) || db_result_affected(result) == 0) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Shipping zone not found");
        return;
    }
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_response(true, "Shipping zone updated");
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}

/* DELETE /api/admin/shipping/zones/:id */
void handler_admin_shipping_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *id_str = http_request_get_path_param(req, "id");

    if (id_str == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Zone ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *params[] = { id_str };
    PGresult *result = db_exec_params(conn,
        "DELETE FROM shipping_zones WHERE id = $1", 1, params);

    if (!db_result_ok(result) || db_result_affected(result) == 0) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Shipping zone not found");
        return;
    }
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *response = json_create_response(true, "Shipping zone deleted");
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);
}
