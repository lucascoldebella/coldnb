#include "handlers/handler_admin_homepage.h"
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

    /* Regular admins need explicit permission */
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

/* Helper to build product JSON for public endpoint */
static cJSON *build_product_json(PGconn *conn, int product_id) {
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%d", product_id);
    const char *params[] = { id_str };

    const char *query =
        "SELECT p.id, p.name, p.slug, p.price, p.compare_at_price, "
        "p.is_new, p.is_sale, "
        "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS primary_image_url "
        "FROM products p WHERE p.id = $1 AND p.is_active = true";

    PGresult *result = db_exec_params(conn, query, 1, params);
    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        return NULL;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *product = db_row_to_json(&row);
    PQclear(result);
    return product;
}

void handler_admin_homepage_register(HttpRouter *router, DbPool *pool) {
    /* Public endpoint */
    ROUTE_GET(router, "/api/homepage", handler_homepage_public, pool);

    /* Admin hero slides */
    ROUTE_GET(router, "/api/admin/homepage/hero-slides", handler_admin_hero_slides_list, pool);
    ROUTE_POST(router, "/api/admin/homepage/hero-slides", handler_admin_hero_slides_create, pool);
    ROUTE_PUT(router, "/api/admin/homepage/hero-slides/reorder", handler_admin_hero_slides_reorder, pool);
    ROUTE_PUT(router, "/api/admin/homepage/hero-slides/:id", handler_admin_hero_slides_update, pool);
    ROUTE_DELETE(router, "/api/admin/homepage/hero-slides/:id", handler_admin_hero_slides_delete, pool);

    /* Admin banners */
    ROUTE_GET(router, "/api/admin/homepage/banners", handler_admin_banners_list, pool);
    ROUTE_POST(router, "/api/admin/homepage/banners", handler_admin_banners_create, pool);
    ROUTE_PUT(router, "/api/admin/homepage/banners/:id", handler_admin_banners_update, pool);
    ROUTE_DELETE(router, "/api/admin/homepage/banners/:id", handler_admin_banners_delete, pool);

    /* Admin sections */
    ROUTE_GET(router, "/api/admin/homepage/sections", handler_admin_sections_list, pool);
    ROUTE_PUT(router, "/api/admin/homepage/sections/:id", handler_admin_sections_update, pool);
    ROUTE_POST(router, "/api/admin/homepage/sections/:id/products", handler_admin_sections_products, pool);

    /* Admin campaigns */
    ROUTE_GET(router, "/api/admin/homepage/campaigns", handler_admin_campaigns_list, pool);
    ROUTE_POST(router, "/api/admin/homepage/campaigns", handler_admin_campaigns_create, pool);
    ROUTE_PUT(router, "/api/admin/homepage/campaigns/:id", handler_admin_campaigns_update, pool);
    ROUTE_DELETE(router, "/api/admin/homepage/campaigns/:id", handler_admin_campaigns_delete, pool);
}

/* ==========================================================================
 * PUBLIC ENDPOINT
 * ========================================================================== */

void handler_homepage_public(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    DbPool *pool = (DbPool *)user_data;

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    cJSON *data = cJSON_CreateObject();

    /* Hero slides - active, within schedule */
    {
        const char *query =
            "SELECT id, title, subtitle, button_text, button_link, image_url, image_alt "
            "FROM homepage_hero_slides "
            "WHERE is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (ends_at IS NULL OR ends_at > NOW()) "
            "ORDER BY sort_order, id";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result)) {
            cJSON_AddItemToObject(data, "hero_slides", db_result_to_json(result));
        } else {
            cJSON_AddItemToObject(data, "hero_slides", cJSON_CreateArray());
        }
        PQclear(result);
    }

    /* Categories for homepage carousel */
    {
        const char *query =
            "SELECT c.id, c.name, c.slug, c.image_url, "
            "(SELECT COUNT(*) FROM products p WHERE p.category_id = c.id AND p.is_active = true) AS product_count "
            "FROM categories c WHERE c.is_active = true "
            "ORDER BY c.sort_order, c.name";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result)) {
            cJSON_AddItemToObject(data, "categories", db_result_to_json(result));
        } else {
            cJSON_AddItemToObject(data, "categories", cJSON_CreateArray());
        }
        PQclear(result);
    }

    /* Collection banners */
    {
        const char *query =
            "SELECT id, title, subtitle, image_url, text_color, position, button_text, button_link "
            "FROM homepage_banners "
            "WHERE banner_type = 'collection' AND is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (ends_at IS NULL OR ends_at > NOW()) "
            "ORDER BY sort_order, id";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result)) {
            cJSON_AddItemToObject(data, "banners_collection", db_result_to_json(result));
        } else {
            cJSON_AddItemToObject(data, "banners_collection", cJSON_CreateArray());
        }
        PQclear(result);
    }

    /* Countdown banner (single active) */
    {
        const char *query =
            "SELECT id, title, discount_label, countdown_end_at, image_url, "
            "button_text, button_link "
            "FROM homepage_banners "
            "WHERE banner_type = 'countdown' AND is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (ends_at IS NULL OR ends_at > NOW()) "
            "ORDER BY sort_order LIMIT 1";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result) && db_result_has_rows(result)) {
            DbRow row = { .result = result, .row = 0 };
            cJSON_AddItemToObject(data, "banner_countdown", db_row_to_json(&row));
        } else {
            cJSON_AddNullToObject(data, "banner_countdown");
        }
        PQclear(result);
    }

    /* Sections */
    {
        cJSON *sections = cJSON_CreateObject();

        const char *query =
            "SELECT id, section_key, title, subtitle, source_type, category_id, "
            "max_items, config "
            "FROM homepage_sections "
            "WHERE is_active = true "
            "ORDER BY sort_order, id";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result)) {
            int nrows = PQntuples(result);
            for (int i = 0; i < nrows; i++) {
                DbRow row = { .result = result, .row = i };
                const char *section_key = db_row_get_string(&row, "section_key");
                const char *source_type = db_row_get_string(&row, "source_type");
                int section_id = db_row_get_int(&row, "id");
                int max_items = db_row_get_int(&row, "max_items");
                if (max_items <= 0) {
                    max_items = 8;
                }

                cJSON *section = cJSON_CreateObject();
                json_add_string_if(section, "title", db_row_get_string(&row, "title"));
                json_add_string_if(section, "subtitle", db_row_get_string(&row, "subtitle"));

                const char *config_str = db_row_get_string(&row, "config");
                if (config_str != NULL && config_str[0] != '\0') {
                    cJSON *config = cJSON_Parse(config_str);
                    if (config != NULL) {
                        cJSON_AddItemToObject(section, "config", config);
                    }
                }

                /* Fetch products for this section */
                cJSON *config = cJSON_GetObjectItem(section, "config");
                cJSON *tabs = config ? cJSON_GetObjectItem(config, "tabs") : NULL;

                if (section_key != NULL && strcmp(section_key, "products_tabbed") == 0
                    && tabs != NULL && cJSON_IsArray(tabs) && cJSON_GetArraySize(tabs) > 0) {
                    /* Tabbed section: fetch products per tab */
                    cJSON *tab_products = cJSON_CreateObject();
                    cJSON *tab_item = NULL;
                    cJSON_ArrayForEach(tab_item, tabs) {
                        const char *tab_name = json_get_string(tab_item, "name", NULL);
                        const char *tab_source = json_get_string(tab_item, "source", NULL);
                        if (tab_name == NULL || tab_source == NULL) {
                            continue;
                        }

                        char limit_str[16];
                        snprintf(limit_str, sizeof(limit_str), "%d", max_items);

                        if (strcmp(tab_source, "manual") == 0) {
                            /* Manual products for this tab */
                            char sid_str[16];
                            snprintf(sid_str, sizeof(sid_str), "%d", section_id);
                            const char *tab_params[] = { sid_str, tab_name, limit_str };

                            const char *tab_query =
                                "SELECT p.id, p.name, p.slug, p.price, p.compare_at_price, "
                                "p.is_new, p.is_sale, "
                                "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS primary_image_url "
                                "FROM homepage_section_products sp "
                                "JOIN products p ON sp.product_id = p.id "
                                "WHERE sp.section_id = $1 AND sp.tab_name = $2 AND p.is_active = true "
                                "ORDER BY sp.sort_order LIMIT $3";

                            PGresult *tab_result = db_exec_params(conn, tab_query, 3, tab_params);
                            if (db_result_ok(tab_result)) {
                                cJSON_AddItemToObject(tab_products, tab_name, db_result_to_json(tab_result));
                            } else {
                                cJSON_AddItemToObject(tab_products, tab_name, cJSON_CreateArray());
                            }
                            PQclear(tab_result);
                        } else {
                            /* Auto-sourced products for this tab */
                            const char *where_extra = "";
                            const char *order_by = "p.created_at DESC";

                            if (strcmp(tab_source, "featured") == 0) {
                                where_extra = " AND p.is_featured = true";
                            } else if (strcmp(tab_source, "new") == 0) {
                                where_extra = " AND p.is_new = true";
                            } else if (strcmp(tab_source, "on_sale") == 0) {
                                where_extra = " AND p.is_sale = true";
                            } else if (strcmp(tab_source, "bestseller") == 0) {
                                order_by = "p.created_at DESC";
                            }

                            char tab_query[1024];
                            snprintf(tab_query, sizeof(tab_query),
                                     "SELECT p.id, p.name, p.slug, p.price, p.compare_at_price, "
                                     "p.is_new, p.is_sale, "
                                     "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS primary_image_url "
                                     "FROM products p "
                                     "WHERE p.is_active = true%s "
                                     "ORDER BY %s LIMIT %s",
                                     where_extra, order_by, limit_str);

                            PGresult *tab_result = db_exec_params(conn, tab_query, 0, NULL);
                            if (db_result_ok(tab_result)) {
                                cJSON_AddItemToObject(tab_products, tab_name, db_result_to_json(tab_result));
                            } else {
                                cJSON_AddItemToObject(tab_products, tab_name, cJSON_CreateArray());
                            }
                            PQclear(tab_result);
                        }
                    }
                    cJSON_AddItemToObject(section, "tab_products", tab_products);
                } else {
                    /* Non-tabbed section: fetch products normally */
                    cJSON *products = cJSON_CreateArray();

                    if (source_type != NULL && strcmp(source_type, "manual") == 0) {
                        char sid_str[16], limit_str[16];
                        snprintf(sid_str, sizeof(sid_str), "%d", section_id);
                        snprintf(limit_str, sizeof(limit_str), "%d", max_items);
                        const char *prod_params[] = { sid_str, limit_str };

                        const char *prod_query =
                            "SELECT p.id, p.name, p.slug, p.price, p.compare_at_price, "
                            "p.is_new, p.is_sale, "
                            "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS primary_image_url "
                            "FROM homepage_section_products sp "
                            "JOIN products p ON sp.product_id = p.id "
                            "WHERE sp.section_id = $1 AND p.is_active = true "
                            "ORDER BY sp.sort_order LIMIT $2";

                        PGresult *prod_result = db_exec_params(conn, prod_query, 2, prod_params);
                        if (db_result_ok(prod_result)) {
                            cJSON_Delete(products);
                            products = db_result_to_json(prod_result);
                        }
                        PQclear(prod_result);
                    } else if (source_type != NULL) {
                        char limit_str[16];
                        snprintf(limit_str, sizeof(limit_str), "%d", max_items);

                        char prod_query[1024];
                        const char *where_extra = "";
                        const char *order_by = "p.created_at DESC";

                        if (strcmp(source_type, "featured") == 0) {
                            where_extra = " AND p.is_featured = true";
                        } else if (strcmp(source_type, "new") == 0) {
                            where_extra = " AND p.is_new = true";
                            order_by = "p.created_at DESC";
                        } else if (strcmp(source_type, "on_sale") == 0) {
                            where_extra = " AND p.is_sale = true";
                        } else if (strcmp(source_type, "bestseller") == 0) {
                            order_by = "p.created_at DESC";
                        }

                        snprintf(prod_query, sizeof(prod_query),
                                 "SELECT p.id, p.name, p.slug, p.price, p.compare_at_price, "
                                 "p.is_new, p.is_sale, "
                                 "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS primary_image_url "
                                 "FROM products p "
                                 "WHERE p.is_active = true%s "
                                 "ORDER BY %s LIMIT %s",
                                 where_extra, order_by, limit_str);

                        PGresult *prod_result = db_exec_params(conn, prod_query, 0, NULL);
                        if (db_result_ok(prod_result)) {
                            cJSON_Delete(products);
                            products = db_result_to_json(prod_result);
                        }
                        PQclear(prod_result);
                    }

                    cJSON_AddItemToObject(section, "products", products);
                }

                if (section_key != NULL) {
                    cJSON_AddItemToObject(sections, section_key, section);
                } else {
                    cJSON_Delete(section);
                }
            }
        }
        PQclear(result);
        cJSON_AddItemToObject(data, "sections", sections);
    }

    /* Active campaign */
    {
        const char *query =
            "SELECT name, ends_at FROM homepage_campaigns "
            "WHERE is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (ends_at IS NULL OR ends_at > NOW()) "
            "ORDER BY created_at DESC LIMIT 1";

        PGresult *result = db_exec_params(conn, query, 0, NULL);
        if (db_result_ok(result) && db_result_has_rows(result)) {
            DbRow row = { .result = result, .row = 0 };
            cJSON_AddItemToObject(data, "active_campaign", db_row_to_json(&row));
        } else {
            cJSON_AddNullToObject(data, "active_campaign");
        }
        PQclear(result);
    }

    db_pool_release(pool, conn);
    send_success(resp, HTTP_STATUS_OK, data);
}

/* ==========================================================================
 * HERO SLIDES CRUD
 * ========================================================================== */

void handler_admin_hero_slides_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        "SELECT id, title, subtitle, button_text, button_link, image_url, image_alt, "
        "product_id, category_id, sort_order, is_active, starts_at, ends_at, "
        "created_at, updated_at "
        "FROM homepage_hero_slides ORDER BY sort_order, id";

    PGresult *result = db_exec_params(conn, query, 0, NULL);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *slides = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "slides", slides);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_hero_slides_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *image_url = json_get_string(body, "image_url", NULL);
    int product_id = json_get_int(body, "product_id", 0);

    /* Auto-populate image from product if product_id set and no image_url */
    char *auto_image = NULL;
    if (product_id > 0 && str_is_empty(image_url)) {
        cJSON *product = build_product_json(conn, product_id);
        if (product != NULL) {
            const char *pimg = json_get_string(product, "primary_image_url", NULL);
            if (pimg != NULL) {
                auto_image = str_dup(pimg);
            }
            cJSON_Delete(product);
        }
    }

    const char *insert_query =
        "INSERT INTO homepage_hero_slides "
        "(title, subtitle, button_text, button_link, image_url, image_alt, "
        "product_id, category_id, sort_order, is_active, starts_at, ends_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12) "
        "RETURNING *";

    char sort_str[16], product_str[16], category_str[16];
    int sort_order = json_get_int(body, "sort_order", 0);
    snprintf(sort_str, sizeof(sort_str), "%d", sort_order);

    const char *product_param = NULL;
    if (product_id > 0) {
        snprintf(product_str, sizeof(product_str), "%d", product_id);
        product_param = product_str;
    }

    int category_id = json_get_int(body, "category_id", 0);
    const char *category_param = NULL;
    if (category_id > 0) {
        snprintf(category_str, sizeof(category_str), "%d", category_id);
        category_param = category_str;
    }

    const char *final_image = auto_image ? auto_image : image_url;

    const char *params[] = {
        json_get_string(body, "title", NULL),
        json_get_string(body, "subtitle", NULL),
        json_get_string(body, "button_text", NULL),
        json_get_string(body, "button_link", NULL),
        final_image,
        json_get_string(body, "image_alt", NULL),
        product_param,
        category_param,
        sort_str,
        json_get_bool(body, "is_active", true) ? "true" : "false",
        json_get_string(body, "starts_at", NULL),
        json_get_string(body, "ends_at", NULL)
    };

    PGresult *result = db_exec_params(conn, insert_query, 12, params);
    free(auto_image);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create slide");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Hero slide created");
}

void handler_admin_hero_slides_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *slide_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (slide_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Slide ID required");
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
        "UPDATE homepage_hero_slides SET "
        "title = COALESCE($2, title), "
        "subtitle = COALESCE($3, subtitle), "
        "button_text = COALESCE($4, button_text), "
        "button_link = COALESCE($5, button_link), "
        "image_url = COALESCE($6, image_url), "
        "image_alt = COALESCE($7, image_alt), "
        "product_id = COALESCE($8, product_id), "
        "category_id = COALESCE($9, category_id), "
        "sort_order = COALESCE($10, sort_order), "
        "is_active = COALESCE($11, is_active), "
        "starts_at = COALESCE($12, starts_at), "
        "ends_at = COALESCE($13, ends_at) "
        "WHERE id = $1 RETURNING *";

    char sort_str[16], product_str[16], category_str[16];

    const char *sort_param = NULL;
    if (cJSON_HasObjectItem(body, "sort_order")) {
        snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
        sort_param = sort_str;
    }

    const char *product_param = NULL;
    if (cJSON_HasObjectItem(body, "product_id")) {
        int pid = json_get_int(body, "product_id", 0);
        if (pid > 0) {
            snprintf(product_str, sizeof(product_str), "%d", pid);
            product_param = product_str;
        }
    }

    const char *category_param = NULL;
    if (cJSON_HasObjectItem(body, "category_id")) {
        int cid = json_get_int(body, "category_id", 0);
        if (cid > 0) {
            snprintf(category_str, sizeof(category_str), "%d", cid);
            category_param = category_str;
        }
    }

    const char *is_active_str = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        slide_id,
        json_get_string(body, "title", NULL),
        json_get_string(body, "subtitle", NULL),
        json_get_string(body, "button_text", NULL),
        json_get_string(body, "button_link", NULL),
        json_get_string(body, "image_url", NULL),
        json_get_string(body, "image_alt", NULL),
        product_param,
        category_param,
        sort_param,
        is_active_str,
        json_get_string(body, "starts_at", NULL),
        json_get_string(body, "ends_at", NULL)
    };

    PGresult *result = db_exec_params(conn, update_query, 13, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Slide not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
    LOG_INFO("Hero slide updated: %s", slide_id);
}

void handler_admin_hero_slides_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *slide_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (slide_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Slide ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query = "DELETE FROM homepage_hero_slides WHERE id = $1 RETURNING id";
    const char *params[] = { slide_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Slide not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
    LOG_INFO("Hero slide deleted: %s", slide_id);
}

void handler_admin_hero_slides_reorder(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "order array required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    if (!db_begin(conn)) {
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, order) {
        int id = json_get_int(item, "id", 0);
        int sort = json_get_int(item, "sort_order", 0);
        if (id <= 0) {
            continue;
        }

        char id_str[16], sort_str[16];
        snprintf(id_str, sizeof(id_str), "%d", id);
        snprintf(sort_str, sizeof(sort_str), "%d", sort);
        const char *params[] = { sort_str, id_str };

        const char *query = "UPDATE homepage_hero_slides SET sort_order = $1 WHERE id = $2";
        PGresult *result = db_exec_params(conn, query, 2, params);
        PQclear(result);
    }

    db_commit(conn);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "reordered", cJSON_True);
    send_success(resp, HTTP_STATUS_OK, data);
}

/* ==========================================================================
 * BANNERS CRUD
 * ========================================================================== */

void handler_admin_banners_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        "SELECT id, title, subtitle, button_text, button_link, image_url, image_alt, "
        "banner_type, text_color, position, countdown_end_at, discount_label, "
        "product_id, category_id, sort_order, is_active, starts_at, ends_at, "
        "created_at, updated_at "
        "FROM homepage_banners ORDER BY banner_type, sort_order, id";

    PGresult *result = db_exec_params(conn, query, 0, NULL);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *banners = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "banners", banners);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_banners_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
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

    const char *banner_type = json_get_string(body, "banner_type", "collection");
    if (strcmp(banner_type, "collection") != 0 && strcmp(banner_type, "countdown") != 0) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid banner_type");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *insert_query =
        "INSERT INTO homepage_banners "
        "(title, subtitle, button_text, button_link, image_url, image_alt, "
        "banner_type, text_color, position, countdown_end_at, discount_label, "
        "product_id, category_id, sort_order, is_active, starts_at, ends_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17) "
        "RETURNING *";

    char sort_str[16], product_str[16], category_str[16];
    snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));

    int product_id = json_get_int(body, "product_id", 0);
    const char *product_param = NULL;
    if (product_id > 0) {
        snprintf(product_str, sizeof(product_str), "%d", product_id);
        product_param = product_str;
    }

    int category_id = json_get_int(body, "category_id", 0);
    const char *category_param = NULL;
    if (category_id > 0) {
        snprintf(category_str, sizeof(category_str), "%d", category_id);
        category_param = category_str;
    }

    const char *params[] = {
        json_get_string(body, "title", NULL),
        json_get_string(body, "subtitle", NULL),
        json_get_string(body, "button_text", NULL),
        json_get_string(body, "button_link", NULL),
        json_get_string(body, "image_url", NULL),
        json_get_string(body, "image_alt", NULL),
        banner_type,
        json_get_string(body, "text_color", "dark"),
        json_get_string(body, "position", "left"),
        json_get_string(body, "countdown_end_at", NULL),
        json_get_string(body, "discount_label", NULL),
        product_param,
        category_param,
        sort_str,
        json_get_bool(body, "is_active", true) ? "true" : "false",
        json_get_string(body, "starts_at", NULL),
        json_get_string(body, "ends_at", NULL)
    };

    PGresult *result = db_exec_params(conn, insert_query, 17, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create banner");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Banner created: %s", banner_type);
}

void handler_admin_banners_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *banner_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (banner_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Banner ID required");
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
        "UPDATE homepage_banners SET "
        "title = COALESCE($2, title), "
        "subtitle = COALESCE($3, subtitle), "
        "button_text = COALESCE($4, button_text), "
        "button_link = COALESCE($5, button_link), "
        "image_url = COALESCE($6, image_url), "
        "image_alt = COALESCE($7, image_alt), "
        "text_color = COALESCE($8, text_color), "
        "position = COALESCE($9, position), "
        "countdown_end_at = COALESCE($10, countdown_end_at), "
        "discount_label = COALESCE($11, discount_label), "
        "sort_order = COALESCE($12, sort_order), "
        "is_active = COALESCE($13, is_active) "
        "WHERE id = $1 RETURNING *";

    char sort_str[16];
    const char *sort_param = NULL;
    if (cJSON_HasObjectItem(body, "sort_order")) {
        snprintf(sort_str, sizeof(sort_str), "%d", json_get_int(body, "sort_order", 0));
        sort_param = sort_str;
    }

    const char *is_active_str = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        banner_id,
        json_get_string(body, "title", NULL),
        json_get_string(body, "subtitle", NULL),
        json_get_string(body, "button_text", NULL),
        json_get_string(body, "button_link", NULL),
        json_get_string(body, "image_url", NULL),
        json_get_string(body, "image_alt", NULL),
        json_get_string(body, "text_color", NULL),
        json_get_string(body, "position", NULL),
        json_get_string(body, "countdown_end_at", NULL),
        json_get_string(body, "discount_label", NULL),
        sort_param,
        is_active_str
    };

    PGresult *result = db_exec_params(conn, update_query, 13, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Banner not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
    LOG_INFO("Banner updated: %s", banner_id);
}

void handler_admin_banners_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *banner_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (banner_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Banner ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query = "DELETE FROM homepage_banners WHERE id = $1 RETURNING id";
    const char *params[] = { banner_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Banner not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
    LOG_INFO("Banner deleted: %s", banner_id);
}

/* ==========================================================================
 * SECTIONS CRUD
 * ========================================================================== */

void handler_admin_sections_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        "SELECT id, section_key, title, subtitle, source_type, category_id, "
        "max_items, config, sort_order, is_active, created_at, updated_at "
        "FROM homepage_sections ORDER BY sort_order, id";

    PGresult *result = db_exec_params(conn, query, 0, NULL);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *sections = cJSON_CreateArray();
    int nrows = PQntuples(result);

    for (int i = 0; i < nrows; i++) {
        DbRow row = { .result = result, .row = i };
        cJSON *section = db_row_to_json(&row);
        int section_id = db_row_get_int(&row, "id");

        /* Fetch manual products for this section */
        char sid_str[16];
        snprintf(sid_str, sizeof(sid_str), "%d", section_id);
        const char *prod_params[] = { sid_str };

        const char *prod_query =
            "SELECT sp.id, sp.product_id, sp.tab_name, sp.sort_order, "
            "p.name AS product_name, "
            "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS product_image "
            "FROM homepage_section_products sp "
            "JOIN products p ON sp.product_id = p.id "
            "WHERE sp.section_id = $1 "
            "ORDER BY sp.sort_order";

        PGresult *prod_result = db_exec_params(conn, prod_query, 1, prod_params);
        if (db_result_ok(prod_result)) {
            cJSON_AddItemToObject(section, "products", db_result_to_json(prod_result));
        } else {
            cJSON_AddItemToObject(section, "products", cJSON_CreateArray());
        }
        PQclear(prod_result);

        cJSON_AddItemToArray(sections, section);
    }

    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "sections", sections);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_sections_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *section_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (section_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Section ID required");
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

    /* Build config JSON string if provided */
    char *config_str = NULL;
    cJSON *config_obj = json_get_object(body, "config");
    if (config_obj != NULL) {
        config_str = cJSON_PrintUnformatted(config_obj);
    }

    const char *update_query =
        "UPDATE homepage_sections SET "
        "title = COALESCE($2, title), "
        "subtitle = COALESCE($3, subtitle), "
        "source_type = COALESCE($4, source_type), "
        "category_id = COALESCE($5, category_id), "
        "max_items = COALESCE($6, max_items), "
        "config = COALESCE($7::jsonb, config), "
        "is_active = COALESCE($8, is_active) "
        "WHERE id = $1 RETURNING *";

    char category_str[16], max_str[16];
    const char *category_param = NULL;
    if (cJSON_HasObjectItem(body, "category_id")) {
        int cid = json_get_int(body, "category_id", 0);
        if (cid > 0) {
            snprintf(category_str, sizeof(category_str), "%d", cid);
            category_param = category_str;
        }
    }

    const char *max_param = NULL;
    if (cJSON_HasObjectItem(body, "max_items")) {
        snprintf(max_str, sizeof(max_str), "%d", json_get_int(body, "max_items", 8));
        max_param = max_str;
    }

    const char *is_active_str = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", true) ? "true" : "false";
    }

    const char *params[] = {
        section_id,
        json_get_string(body, "title", NULL),
        json_get_string(body, "subtitle", NULL),
        json_get_string(body, "source_type", NULL),
        category_param,
        max_param,
        config_str,
        is_active_str
    };

    PGresult *result = db_exec_params(conn, update_query, 8, params);
    free(config_str);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Section not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
    LOG_INFO("Section updated: %s", section_id);
}

void handler_admin_sections_products(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *section_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (section_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Section ID required");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    cJSON *products_arr = json_get_array(body, "products");
    if (products_arr == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "products array required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    if (!db_begin(conn)) {
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* Delete existing products for this section */
    const char *delete_params[] = { section_id };
    PGresult *del_result = db_exec_params(conn,
        "DELETE FROM homepage_section_products WHERE section_id = $1", 1, delete_params);
    PQclear(del_result);

    /* Insert new products */
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, products_arr) {
        int product_id = json_get_int(item, "product_id", 0);
        if (product_id <= 0) {
            continue;
        }

        const char *tab_name = json_get_string(item, "tab_name", NULL);
        int sort_order = json_get_int(item, "sort_order", 0);

        char pid_str[16], sort_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", product_id);
        snprintf(sort_str, sizeof(sort_str), "%d", sort_order);

        const char *insert_params[] = { section_id, pid_str, tab_name, sort_str };
        PGresult *ins_result = db_exec_params(conn,
            "INSERT INTO homepage_section_products (section_id, product_id, tab_name, sort_order) "
            "VALUES ($1, $2, $3, $4) ON CONFLICT DO NOTHING",
            4, insert_params);
        PQclear(ins_result);
    }

    db_commit(conn);
    db_pool_release(pool, conn);
    cJSON_Delete(body);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "updated", cJSON_True);
    send_success(resp, HTTP_STATUS_OK, data);
    LOG_INFO("Section products updated: %s", section_id);
}

/* ==========================================================================
 * CAMPAIGNS CRUD
 * ========================================================================== */

void handler_admin_campaigns_list(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
        "SELECT id, name, description, starts_at, ends_at, is_active, "
        "hero_slide_ids, banner_ids, created_at, updated_at "
        "FROM homepage_campaigns ORDER BY created_at DESC";

    PGresult *result = db_exec_params(conn, query, 0, NULL);
    if (!db_result_ok(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Query failed");
        return;
    }

    cJSON *campaigns = db_result_to_json(result);
    PQclear(result);
    db_pool_release(pool, conn);

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "campaigns", campaigns);
    send_success(resp, HTTP_STATUS_OK, data);
}

void handler_admin_campaigns_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
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
    if (str_is_empty(name)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Name required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Build hero_slide_ids and banner_ids arrays as PostgreSQL array literals */
    char hero_ids_str[256] = "{}";
    cJSON *hero_arr = json_get_array(body, "hero_slide_ids");
    if (hero_arr != NULL && cJSON_GetArraySize(hero_arr) > 0) {
        size_t pos = 0;
        pos = (size_t)snprintf(hero_ids_str, sizeof(hero_ids_str), "{");
        cJSON *h = NULL;
        bool first = true;
        cJSON_ArrayForEach(h, hero_arr) {
            if (cJSON_IsNumber(h)) {
                if (!first && pos < sizeof(hero_ids_str) - 1) {
                    pos += (size_t)snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, ",");
                }
                pos += (size_t)snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, "%d", h->valueint);
                first = false;
            }
        }
        if (pos < sizeof(hero_ids_str) - 1) {
            snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, "}");
        }
    }

    char banner_ids_str[256] = "{}";
    cJSON *banner_arr = json_get_array(body, "banner_ids");
    if (banner_arr != NULL && cJSON_GetArraySize(banner_arr) > 0) {
        size_t pos = 0;
        pos = (size_t)snprintf(banner_ids_str, sizeof(banner_ids_str), "{");
        cJSON *b = NULL;
        bool first = true;
        cJSON_ArrayForEach(b, banner_arr) {
            if (cJSON_IsNumber(b)) {
                if (!first && pos < sizeof(banner_ids_str) - 1) {
                    pos += (size_t)snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, ",");
                }
                pos += (size_t)snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, "%d", b->valueint);
                first = false;
            }
        }
        if (pos < sizeof(banner_ids_str) - 1) {
            snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, "}");
        }
    }

    const char *insert_query =
        "INSERT INTO homepage_campaigns "
        "(name, description, starts_at, ends_at, is_active, hero_slide_ids, banner_ids) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) "
        "RETURNING *";

    const char *params[] = {
        name,
        json_get_string(body, "description", NULL),
        json_get_string(body, "starts_at", NULL),
        json_get_string(body, "ends_at", NULL),
        json_get_bool(body, "is_active", false) ? "true" : "false",
        hero_ids_str,
        banner_ids_str
    };

    PGresult *result = db_exec_params(conn, insert_query, 7, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create campaign");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_CREATED, data);
    LOG_INFO("Campaign created: %s", name);
}

void handler_admin_campaigns_update(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *campaign_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (campaign_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Campaign ID required");
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
        "UPDATE homepage_campaigns SET "
        "name = COALESCE($2, name), "
        "description = COALESCE($3, description), "
        "starts_at = COALESCE($4, starts_at), "
        "ends_at = COALESCE($5, ends_at), "
        "is_active = COALESCE($6, is_active), "
        "hero_slide_ids = COALESCE($7, hero_slide_ids), "
        "banner_ids = COALESCE($8, banner_ids) "
        "WHERE id = $1 RETURNING *";

    const char *is_active_str = NULL;
    if (cJSON_HasObjectItem(body, "is_active")) {
        is_active_str = json_get_bool(body, "is_active", false) ? "true" : "false";
    }

    /* Build array params only if provided */
    char hero_ids_str[256];
    const char *hero_param = NULL;
    cJSON *hero_arr = json_get_array(body, "hero_slide_ids");
    if (hero_arr != NULL) {
        size_t pos = 0;
        pos = (size_t)snprintf(hero_ids_str, sizeof(hero_ids_str), "{");
        cJSON *h = NULL;
        bool first = true;
        cJSON_ArrayForEach(h, hero_arr) {
            if (cJSON_IsNumber(h)) {
                if (!first && pos < sizeof(hero_ids_str) - 1) {
                    pos += (size_t)snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, ",");
                }
                pos += (size_t)snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, "%d", h->valueint);
                first = false;
            }
        }
        if (pos < sizeof(hero_ids_str) - 1) {
            snprintf(hero_ids_str + pos, sizeof(hero_ids_str) - pos, "}");
        }
        hero_param = hero_ids_str;
    }

    char banner_ids_str[256];
    const char *banner_param = NULL;
    cJSON *banner_arr = json_get_array(body, "banner_ids");
    if (banner_arr != NULL) {
        size_t pos = 0;
        pos = (size_t)snprintf(banner_ids_str, sizeof(banner_ids_str), "{");
        cJSON *b = NULL;
        bool first = true;
        cJSON_ArrayForEach(b, banner_arr) {
            if (cJSON_IsNumber(b)) {
                if (!first && pos < sizeof(banner_ids_str) - 1) {
                    pos += (size_t)snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, ",");
                }
                pos += (size_t)snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, "%d", b->valueint);
                first = false;
            }
        }
        if (pos < sizeof(banner_ids_str) - 1) {
            snprintf(banner_ids_str + pos, sizeof(banner_ids_str) - pos, "}");
        }
        banner_param = banner_ids_str;
    }

    const char *params[] = {
        campaign_id,
        json_get_string(body, "name", NULL),
        json_get_string(body, "description", NULL),
        json_get_string(body, "starts_at", NULL),
        json_get_string(body, "ends_at", NULL),
        is_active_str,
        hero_param,
        banner_param
    };

    PGresult *result = db_exec_params(conn, update_query, 8, params);
    cJSON_Delete(body);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Campaign not found");
        return;
    }

    DbRow row = { .result = result, .row = 0 };
    cJSON *data = db_row_to_json(&row);
    PQclear(result);
    db_pool_release(pool, conn);

    send_success(resp, HTTP_STATUS_OK, data);
    LOG_INFO("Campaign updated: %s", campaign_id);
}

void handler_admin_campaigns_delete(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *campaign_id = http_request_get_path_param(req, "id");

    if (!has_permission(req, pool, "manage_homepage")) {
        http_response_error(resp, HTTP_STATUS_FORBIDDEN, "Permission denied");
        return;
    }

    if (campaign_id == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Campaign ID required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    const char *query = "DELETE FROM homepage_campaigns WHERE id = $1 RETURNING id";
    const char *params[] = { campaign_id };
    PGresult *result = db_exec_params(conn, query, 1, params);

    if (!db_result_ok(result) || !db_result_has_rows(result)) {
        PQclear(result);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Campaign not found");
        return;
    }

    PQclear(result);
    db_pool_release(pool, conn);

    http_response_no_content(resp);
    LOG_INFO("Campaign deleted: %s", campaign_id);
}
