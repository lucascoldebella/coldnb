#include "handlers/handler_admin_navigation.h"
#include "auth/auth_middleware.h"
#include "services/svc_admin_auth.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

/* Helper to check permission */
static bool has_permission(HttpRequest *req, DbPool *pool, const char *permission) {
    (void)permission;
    const char *token = http_request_get_bearer_token(req);
    if (token == NULL) {
        return false;
    }

    AdminUser *user = admin_auth_validate_token(pool, token);
    if (user == NULL) {
        return false;
    }

    /* super_admin bypasses all permission checks */
    if (strcmp(user->role, "super_admin") == 0) {
        admin_user_free(user);
        return true;
    }

    admin_user_free(user);
    return false;
}

/* Helper to send JSON success response */
static void send_success(HttpResponse *resp, HttpStatus status, cJSON *data) {
    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    http_response_json(resp, status, json);
    free(json);
}

void handler_admin_navigation_register(HttpRouter *router, DbPool *pool) {
    /* Public endpoint */
    ROUTE_GET(router, "/api/navigation", handler_navigation_public, pool);

    /* Admin menus */
    ROUTE_GET(router, "/api/admin/navigation/menus", handler_admin_nav_menus_list, pool);
    ROUTE_POST(router, "/api/admin/navigation/menus", handler_admin_nav_menus_create, pool);
    ROUTE_PUT(router, "/api/admin/navigation/menus/:id", handler_admin_nav_menus_update, pool);
    ROUTE_DELETE(router, "/api/admin/navigation/menus/:id", handler_admin_nav_menus_delete, pool);

    /* Admin groups */
    ROUTE_GET(router, "/api/admin/navigation/menus/:id/groups", handler_admin_nav_groups_list, pool);
    ROUTE_POST(router, "/api/admin/navigation/menus/:id/groups", handler_admin_nav_groups_create, pool);
    ROUTE_PUT(router, "/api/admin/navigation/groups/:id", handler_admin_nav_groups_update, pool);
    ROUTE_DELETE(router, "/api/admin/navigation/groups/:id", handler_admin_nav_groups_delete, pool);

    /* Admin items */
    ROUTE_POST(router, "/api/admin/navigation/groups/:id/items", handler_admin_nav_items_create, pool);
    ROUTE_PUT(router, "/api/admin/navigation/items/:id", handler_admin_nav_items_update, pool);
    ROUTE_DELETE(router, "/api/admin/navigation/items/:id", handler_admin_nav_items_delete, pool);
    ROUTE_PUT(router, "/api/admin/navigation/items/reorder", handler_admin_nav_items_reorder, pool);
}

/* ==========================================================================
 * PUBLIC ENDPOINT
 * ========================================================================== */

void handler_navigation_public(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    DbPool *pool = (DbPool *)user_data;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Fetch all active menus */
    const char *menus_query =
        "SELECT id, name, slug, menu_type, show_products, products_count, "
        "banner_image_url, banner_link, banner_title, translation_key, sort_order "
        "FROM navigation_menus WHERE is_active = true ORDER BY sort_order, id";

    PGresult *menus_result = db_exec_params(conn, menus_query, 0, NULL);
    if (!db_result_ok(menus_result)) {
        PQclear(menus_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    /* Fetch all active groups */
    const char *groups_query =
        "SELECT g.id, g.menu_id, g.title, g.translation_key, g.sort_order "
        "FROM navigation_groups g "
        "JOIN navigation_menus m ON g.menu_id = m.id "
        "WHERE g.is_active = true AND m.is_active = true "
        "ORDER BY g.menu_id, g.sort_order, g.id";

    PGresult *groups_result = db_exec_params(conn, groups_query, 0, NULL);
    if (!db_result_ok(groups_result)) {
        PQclear(groups_result);
        PQclear(menus_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    /* Fetch all active items */
    const char *items_query =
        "SELECT i.id, i.group_id, i.label, i.href, i.image_url, i.image_alt, "
        "i.badge, i.sort_order "
        "FROM navigation_items i "
        "JOIN navigation_groups g ON i.group_id = g.id "
        "JOIN navigation_menus m ON g.menu_id = m.id "
        "WHERE i.is_active = true AND g.is_active = true AND m.is_active = true "
        "ORDER BY i.group_id, i.sort_order, i.id";

    PGresult *items_result = db_exec_params(conn, items_query, 0, NULL);
    if (!db_result_ok(items_result)) {
        PQclear(items_result);
        PQclear(groups_result);
        PQclear(menus_result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    db_pool_release(pool, conn);

    /* Build nested JSON: menus -> groups -> items */
    cJSON *menus_arr = cJSON_CreateArray();

    int num_menus = PQntuples(menus_result);
    int num_groups = PQntuples(groups_result);
    int num_items = PQntuples(items_result);

    /* Column indices for items */
    int item_col_id = PQfnumber(items_result, "id");
    int item_col_group_id = PQfnumber(items_result, "group_id");
    int item_col_label = PQfnumber(items_result, "label");
    int item_col_href = PQfnumber(items_result, "href");
    int item_col_image_url = PQfnumber(items_result, "image_url");
    int item_col_image_alt = PQfnumber(items_result, "image_alt");
    int item_col_badge = PQfnumber(items_result, "badge");
    int item_col_sort = PQfnumber(items_result, "sort_order");

    /* Column indices for groups */
    int grp_col_id = PQfnumber(groups_result, "id");
    int grp_col_menu_id = PQfnumber(groups_result, "menu_id");
    int grp_col_title = PQfnumber(groups_result, "title");
    int grp_col_translation_key = PQfnumber(groups_result, "translation_key");
    int grp_col_sort = PQfnumber(groups_result, "sort_order");

    /* Column indices for menus */
    int menu_col_id = PQfnumber(menus_result, "id");
    int menu_col_name = PQfnumber(menus_result, "name");
    int menu_col_slug = PQfnumber(menus_result, "slug");
    int menu_col_type = PQfnumber(menus_result, "menu_type");
    int menu_col_show_products = PQfnumber(menus_result, "show_products");
    int menu_col_products_count = PQfnumber(menus_result, "products_count");
    int menu_col_banner_image = PQfnumber(menus_result, "banner_image_url");
    int menu_col_banner_link = PQfnumber(menus_result, "banner_link");
    int menu_col_banner_title = PQfnumber(menus_result, "banner_title");
    int menu_col_translation_key = PQfnumber(menus_result, "translation_key");

    int item_idx = 0;
    int group_idx = 0;

    for (int m = 0; m < num_menus; m++) {
        int menu_id = atoi(PQgetvalue(menus_result, m, menu_col_id));

        cJSON *menu_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(menu_obj, "id", menu_id);
        cJSON_AddStringToObject(menu_obj, "name", PQgetvalue(menus_result, m, menu_col_name));
        cJSON_AddStringToObject(menu_obj, "slug", PQgetvalue(menus_result, m, menu_col_slug));
        cJSON_AddStringToObject(menu_obj, "menu_type", PQgetvalue(menus_result, m, menu_col_type));

        const char *show_products_str = PQgetvalue(menus_result, m, menu_col_show_products);
        cJSON_AddBoolToObject(menu_obj, "show_products", show_products_str && strcmp(show_products_str, "t") == 0);

        if (!PQgetisnull(menus_result, m, menu_col_products_count)) {
            cJSON_AddNumberToObject(menu_obj, "products_count", atoi(PQgetvalue(menus_result, m, menu_col_products_count)));
        }

        if (!PQgetisnull(menus_result, m, menu_col_banner_image)) {
            cJSON_AddStringToObject(menu_obj, "banner_image_url", PQgetvalue(menus_result, m, menu_col_banner_image));
        }
        if (!PQgetisnull(menus_result, m, menu_col_banner_link)) {
            cJSON_AddStringToObject(menu_obj, "banner_link", PQgetvalue(menus_result, m, menu_col_banner_link));
        }
        if (!PQgetisnull(menus_result, m, menu_col_banner_title)) {
            cJSON_AddStringToObject(menu_obj, "banner_title", PQgetvalue(menus_result, m, menu_col_banner_title));
        }
        if (!PQgetisnull(menus_result, m, menu_col_translation_key)) {
            cJSON_AddStringToObject(menu_obj, "translation_key", PQgetvalue(menus_result, m, menu_col_translation_key));
        }

        cJSON *groups_arr = cJSON_CreateArray();

        /* Iterate groups belonging to this menu */
        while (group_idx < num_groups) {
            int grp_menu_id = atoi(PQgetvalue(groups_result, group_idx, grp_col_menu_id));
            if (grp_menu_id != menu_id) {
                break;
            }

            int group_id = atoi(PQgetvalue(groups_result, group_idx, grp_col_id));
            cJSON *group_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(group_obj, "id", group_id);

            if (!PQgetisnull(groups_result, group_idx, grp_col_title)) {
                cJSON_AddStringToObject(group_obj, "title", PQgetvalue(groups_result, group_idx, grp_col_title));
            } else {
                cJSON_AddNullToObject(group_obj, "title");
            }

            if (!PQgetisnull(groups_result, group_idx, grp_col_translation_key)) {
                cJSON_AddStringToObject(group_obj, "translation_key", PQgetvalue(groups_result, group_idx, grp_col_translation_key));
            }

            cJSON_AddNumberToObject(group_obj, "sort_order", atoi(PQgetvalue(groups_result, group_idx, grp_col_sort)));

            cJSON *items_arr = cJSON_CreateArray();

            /* Iterate items belonging to this group */
            while (item_idx < num_items) {
                int itm_group_id = atoi(PQgetvalue(items_result, item_idx, item_col_group_id));
                if (itm_group_id != group_id) {
                    break;
                }

                cJSON *item_obj = cJSON_CreateObject();
                cJSON_AddNumberToObject(item_obj, "id", atoi(PQgetvalue(items_result, item_idx, item_col_id)));
                cJSON_AddStringToObject(item_obj, "label", PQgetvalue(items_result, item_idx, item_col_label));
                cJSON_AddStringToObject(item_obj, "href", PQgetvalue(items_result, item_idx, item_col_href));

                if (!PQgetisnull(items_result, item_idx, item_col_image_url)) {
                    cJSON_AddStringToObject(item_obj, "image_url", PQgetvalue(items_result, item_idx, item_col_image_url));
                }
                if (!PQgetisnull(items_result, item_idx, item_col_image_alt)) {
                    cJSON_AddStringToObject(item_obj, "image_alt", PQgetvalue(items_result, item_idx, item_col_image_alt));
                }
                if (!PQgetisnull(items_result, item_idx, item_col_badge)) {
                    cJSON_AddStringToObject(item_obj, "badge", PQgetvalue(items_result, item_idx, item_col_badge));
                } else {
                    cJSON_AddNullToObject(item_obj, "badge");
                }
                cJSON_AddNumberToObject(item_obj, "sort_order", atoi(PQgetvalue(items_result, item_idx, item_col_sort)));

                cJSON_AddItemToArray(items_arr, item_obj);
                item_idx++;
            }

            cJSON_AddItemToObject(group_obj, "items", items_arr);
            cJSON_AddItemToArray(groups_arr, group_obj);
            group_idx++;
        }

        cJSON_AddItemToObject(menu_obj, "groups", groups_arr);
        cJSON_AddItemToArray(menus_arr, menu_obj);
    }

    PQclear(items_result);
    PQclear(groups_result);
    PQclear(menus_result);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "menus", menus_arr);
    send_success(resp, HTTP_STATUS_OK, data);
}

/* ==========================================================================
 * MENUS CRUD
 * ========================================================================== */

void handler_admin_nav_menus_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT m.id, m.name, m.slug, m.menu_type, m.show_products, m.products_count, "
        "m.banner_image_url, m.banner_link, m.banner_title, m.translation_key, "
        "m.sort_order, m.is_active, m.created_at, m.updated_at, "
        "(SELECT COUNT(*) FROM navigation_groups g WHERE g.menu_id = m.id) AS group_count, "
        "(SELECT COUNT(*) FROM navigation_items i JOIN navigation_groups g2 ON i.group_id = g2.id WHERE g2.menu_id = m.id) AS item_count "
        "FROM navigation_menus m ORDER BY m.sort_order, m.id";

    PGresult *result = db_exec_params(conn, query, 0, NULL);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *menus = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "menus", menus);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_nav_menus_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *name = json_get_string(body, "name", NULL);
    const char *slug = json_get_string(body, "slug", NULL);
    if (str_is_empty(name) || str_is_empty(slug)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name and slug are required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *insert_query =
        "INSERT INTO navigation_menus "
        "(name, slug, menu_type, show_products, products_count, "
        "banner_image_url, banner_link, banner_title, translation_key, sort_order, is_active) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING *";

    char sort_str[16], count_str[16];
    snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
    snprintf(count_str, sizeof(count_str), "%d", json_get_int(body, "products_count", 4));

    const char *params[] = {
        name, slug,
        json_get_string(body, "menu_type", "simple"),
        json_get_bool(body, "show_products", false) ? "true" : "false",
        count_str,
        json_get_string(body, "banner_image_url", NULL),
        json_get_string(body, "banner_link", NULL),
        json_get_string(body, "banner_title", NULL),
        json_get_string(body, "translation_key", NULL),
        sort_str,
        json_get_bool(body, "is_active", true) ? "true" : "false"
    };

    PGresult *result = db_exec_params(conn, insert_query, 11, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create menu");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Navigation menu created");
}

void handler_admin_nav_menus_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *menu_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (menu_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Menu ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *update_query =
        "UPDATE navigation_menus SET "
        "name = COALESCE($2, name), "
        "slug = COALESCE($3, slug), "
        "menu_type = COALESCE($4, menu_type), "
        "show_products = COALESCE($5, show_products), "
        "products_count = COALESCE($6, products_count), "
        "banner_image_url = COALESCE($7, banner_image_url), "
        "banner_link = COALESCE($8, banner_link), "
        "banner_title = COALESCE($9, banner_title), "
        "translation_key = COALESCE($10, translation_key), "
        "sort_order = COALESCE($11, sort_order), "
        "is_active = COALESCE($12, is_active) "
        "WHERE id = $1 RETURNING *";

    char sort_str[16], count_str[16];

    const char *sort_param = NULL;
    if (cJSON_HasObjectItem(body, "sort_order")) {
        snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
        sort_param = sort_str;
    }

    const char *count_param = NULL;
    if (cJSON_HasObjectItem(body, "products_count")) {
        snprintf(count_str, sizeof(count_str), "%d", json_get_int(body, "products_count", 4));
        count_param = count_str;
    }

    const char *show_products_param = NULL;
    if (cJSON_HasObjectItem(body, "show_products")) {
        show_products_param = json_get_bool(body, "show_products", false) ? "true" : "false";
    }

    const char *is_active_param = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_param = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        menu_id,
        json_get_string(body, "name", NULL),
        json_get_string(body, "slug", NULL),
        json_get_string(body, "menu_type", NULL),
        show_products_param,
        count_param,
        json_get_string(body, "banner_image_url", NULL),
        json_get_string(body, "banner_link", NULL),
        json_get_string(body, "banner_title", NULL),
        json_get_string(body, "translation_key", NULL),
        sort_param,
        is_active_param
    };

    PGresult *result = db_exec_params(conn, update_query, 12, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Menu not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_nav_menus_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *menu_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (menu_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Menu ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *params[] = { menu_id };
    PGresult *result = db_exec_params(conn, "DELETE FROM navigation_menus WHERE id = $1", 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to delete menu");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);
    http_response_json(resp, HTTP_STATUS_NO_CONTENT, "");
}

/* ==========================================================================
 * GROUPS CRUD
 * ========================================================================== */

void handler_admin_nav_groups_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *menu_id = http_request_get_path_param(req, "id");

    if (!auth_is_admin(req)) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Admin access required");
        return;
    }

    if (menu_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Menu ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query =
        "SELECT g.id, g.menu_id, g.title, g.translation_key, g.sort_order, g.is_active, "
        "g.created_at, g.updated_at, "
        "(SELECT COUNT(*) FROM navigation_items i WHERE i.group_id = g.id) AS item_count "
        "FROM navigation_groups g WHERE g.menu_id = $1 ORDER BY g.sort_order, g.id";

    const char *params[] = { menu_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *groups = db_result_to_json(result);
    PQclear(result);

    /* Also fetch items for each group */
    int num_groups = cJSON_GetArraySize(groups);
    for (int i = 0; i < num_groups; i++) {
        cJSON *group = cJSON_GetArrayItem(groups, i);
        cJSON *gid = cJSON_GetObjectItem(group, "id");
        if (gid == NULL) {
            continue;
        }

        char gid_str[16];
        if (cJSON_IsNumber(gid)) {
            snprintf(gid_str, sizeof(gid_str), "%d", (int)gid->valuedouble);
        } else if (cJSON_IsString(gid)) {
            snprintf(gid_str, sizeof(gid_str), "%s", gid->valuestring);
        } else {
            continue;
        }

        const char *items_query =
            "SELECT id, group_id, label, href, image_url, image_alt, badge, sort_order, is_active "
            "FROM navigation_items WHERE group_id = $1 ORDER BY sort_order, id";

        const char *items_params[] = { gid_str };
        PGresult *items_result = db_exec_params(conn, items_query, 1, items_params);

        if (db_result_ok(items_result)) {
            cJSON *items = db_result_to_json(items_result);
            cJSON_AddItemToObject(group, "items", items);
        }
        PQclear(items_result);
    }

    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "groups", groups);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_nav_groups_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *menu_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (menu_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Menu ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *insert_query =
        "INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order, is_active) "
        "VALUES ($1, $2, $3, $4, $5) RETURNING *";

    char sort_str[16];
    snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));

    const char *params[] = {
        menu_id,
        json_get_string(body, "title", NULL),
        json_get_string(body, "translation_key", NULL),
        sort_str,
        json_get_bool(body, "is_active", true) ? "true" : "false"
    };

    PGresult *result = db_exec_params(conn, insert_query, 5, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create group");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Navigation group created");
}

void handler_admin_nav_groups_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *group_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (group_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Group ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *update_query =
        "UPDATE navigation_groups SET "
        "title = COALESCE($2, title), "
        "translation_key = COALESCE($3, translation_key), "
        "sort_order = COALESCE($4, sort_order), "
        "is_active = COALESCE($5, is_active) "
        "WHERE id = $1 RETURNING *";

    char sort_str[16];
    const char *sort_param = NULL;
    if (cJSON_HasObjectItem(body, "sort_order")) {
        snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
        sort_param = sort_str;
    }

    const char *is_active_param = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_param = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        group_id,
        json_get_string(body, "title", NULL),
        json_get_string(body, "translation_key", NULL),
        sort_param,
        is_active_param
    };

    PGresult *result = db_exec_params(conn, update_query, 5, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Group not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_nav_groups_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *group_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (group_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Group ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *params[] = { group_id };
    PGresult *result = db_exec_params(conn, "DELETE FROM navigation_groups WHERE id = $1", 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to delete group");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);
    http_response_json(resp, HTTP_STATUS_NO_CONTENT, "");
}

/* ==========================================================================
 * ITEMS CRUD
 * ========================================================================== */

void handler_admin_nav_items_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *group_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (group_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Group ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *label = json_get_string(body, "label", NULL);
    const char *href = json_get_string(body, "href", NULL);
    if (str_is_empty(label) || str_is_empty(href)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Label and href are required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *insert_query =
        "INSERT INTO navigation_items "
        "(group_id, label, href, image_url, image_alt, badge, sort_order, is_active) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING *";

    char sort_str[16];
    snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));

    const char *params[] = {
        group_id,
        label,
        href,
        json_get_string(body, "image_url", NULL),
        json_get_string(body, "image_alt", NULL),
        json_get_string(body, "badge", NULL),
        sort_str,
        json_get_bool(body, "is_active", true) ? "true" : "false"
    };

    PGresult *result = db_exec_params(conn, insert_query, 8, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create item");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Navigation item created");
}

void handler_admin_nav_items_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *item_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (item_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Item ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *update_query =
        "UPDATE navigation_items SET "
        "label = COALESCE($2, label), "
        "href = COALESCE($3, href), "
        "image_url = COALESCE($4, image_url), "
        "image_alt = COALESCE($5, image_alt), "
        "badge = COALESCE($6, badge), "
        "sort_order = COALESCE($7, sort_order), "
        "is_active = COALESCE($8, is_active) "
        "WHERE id = $1 RETURNING *";

    char sort_str[16];
    const char *sort_param = NULL;
    if (cJSON_HasObjectItem(body, "sort_order")) {
        snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
        sort_param = sort_str;
    }

    const char *is_active_param = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_param = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        item_id,
        json_get_string(body, "label", NULL),
        json_get_string(body, "href", NULL),
        json_get_string(body, "image_url", NULL),
        json_get_string(body, "image_alt", NULL),
        json_get_string(body, "badge", NULL),
        sort_param,
        is_active_param
    };

    PGresult *result = db_exec_params(conn, update_query, 8, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Item not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_nav_items_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *item_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (item_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Item ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *params[] = { item_id };
    PGresult *result = db_exec_params(conn, "DELETE FROM navigation_items WHERE id = $1", 1, params);

    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to delete item");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);
    http_response_json(resp, HTTP_STATUS_NO_CONTENT, "");
}

void handler_admin_nav_items_reorder(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *order = json_get_array(body, "order");
    if (order == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Order array required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    if (!db_begin(conn)) {
        cJSON_Delete(body);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    int size = cJSON_GetArraySize(order);
    for (int i = 0; i < size; i++) {
        cJSON *item_id = cJSON_GetArrayItem(order, i);
        if (!cJSON_IsNumber(item_id)) {
            continue;
        }

        char id_str[16], sort_str[16];
        snprintf(id_str, sizeof(id_str), "%d", (int)item_id->valuedouble);
        snprintf(sort_str, sizeof(sort_str), "%d", i);

        const char *params[] = { sort_str, id_str };
        PGresult *result = db_exec_params(conn,
            "UPDATE navigation_items SET sort_order = $1 WHERE id = $2",
            2, params);

        if (!db_result_ok(result)) {
            PQclear(result);
            db_pool_release(pool, conn);
            cJSON_Delete(body);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Reorder failed");
            return;
        }
        PQclear(result);
    }

    db_commit(conn);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "reordered", true);
    send_success(resp, HTTP_STATUS_OK, data);
}
