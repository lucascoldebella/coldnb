#include "handlers/handler_guest_orders.h"
#include "clients/client_stripe.h"
#include "db/db_query.h"
#include "services/svc_email.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Generate order number: ORD-YYYYMMDD-XXXXX */
static char *guest_generate_order_number(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char *random = str_random(5);
    if (random == NULL) return NULL;

    char *order_num = str_printf("ORD-%04d%02d%02d-%s",
                                 tm_info->tm_year + 1900,
                                 tm_info->tm_mon + 1,
                                 tm_info->tm_mday,
                                 random);
    free(random);
    return order_num;
}

void handler_guest_orders_register(HttpRouter *router, DbPool *pool) {
    ROUTE_POST(router, "/api/guest-orders", handler_guest_orders_create, pool);
    ROUTE_POST(router, "/api/guest-payments/create-intent", handler_guest_payments_create_intent, pool);
}

/*
 * POST /api/guest-orders
 *
 * Body:
 * {
 *   "guest_email": "customer@example.com",
 *   "guest_name": "John Doe",
 *   "phone": "11999999999",
 *   "shipping_street": "Rua das Flores, 123",
 *   "shipping_street_2": "Apto 4",     (optional)
 *   "shipping_city": "São Paulo",
 *   "shipping_state": "SP",
 *   "shipping_postal_code": "01310000",
 *   "shipping_country": "BR",          (defaults to BR)
 *   "notes": "...",                     (optional)
 *   "discount_code": "SAVE10",          (optional)
 *   "items": [
 *     { "product_id": 5, "quantity": 2 }
 *   ]
 * }
 */
void handler_guest_orders_create(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    /* Required fields */
    const char *guest_email   = json_get_string(body, "guest_email", NULL);
    const char *guest_name    = json_get_string(body, "guest_name", NULL);
    const char *phone         = json_get_string(body, "phone", NULL);
    const char *ship_street   = json_get_string(body, "shipping_street", NULL);
    const char *ship_street_2 = json_get_string(body, "shipping_street_2", NULL);
    const char *ship_city     = json_get_string(body, "shipping_city", NULL);
    const char *ship_state    = json_get_string(body, "shipping_state", NULL);
    const char *ship_postal   = json_get_string(body, "shipping_postal_code", NULL);
    const char *ship_country  = json_get_string(body, "shipping_country", "BR");
    const char *notes         = json_get_string(body, "notes", NULL);
    const char *discount_code = json_get_string(body, "discount_code", NULL);

    if (str_is_empty(guest_email) || str_is_empty(guest_name) ||
        str_is_empty(ship_street) || str_is_empty(ship_city) ||
        str_is_empty(ship_state)  || str_is_empty(ship_postal)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST,
                            "guest_email, guest_name, shipping address fields are required");
        return;
    }

    /* Parse items array */
    cJSON *items_json = cJSON_GetObjectItem(body, "items");
    if (items_json == NULL || !cJSON_IsArray(items_json) ||
        cJSON_GetArraySize(items_json) == 0) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "items array is required");
        return;
    }

    int item_count = cJSON_GetArraySize(items_json);

    /* Copy strings before freeing body */
    char *c_email    = str_dup(guest_email);
    char *c_name     = str_dup(guest_name);
    char *c_phone    = phone ? str_dup(phone) : NULL;
    char *c_street   = str_dup(ship_street);
    char *c_street_2 = ship_street_2 ? str_dup(ship_street_2) : NULL;
    char *c_city     = str_dup(ship_city);
    char *c_state    = str_dup(ship_state);
    char *c_postal   = str_dup(ship_postal);
    char *c_country  = str_dup(ship_country ? ship_country : "BR");
    char *c_notes    = notes ? str_dup(notes) : NULL;

    /* Parse items into a local array */
    int *product_ids  = calloc(item_count, sizeof(int));
    int *quantities   = calloc(item_count, sizeof(int));

    if (!product_ids || !quantities) {
        free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Memory allocation failed");
        return;
    }

    for (int i = 0; i < item_count; i++) {
        cJSON *item = cJSON_GetArrayItem(items_json, i);
        product_ids[i] = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(item, "product_id"));
        quantities[i]  = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(item, "quantity"));
        if (quantities[i] < 1) quantities[i] = 1;
    }

    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Validate products and calculate subtotal */
    double subtotal = 0;
    typedef struct { int id; char name[256]; char sku[64]; char image[512];
                     double price; int stock; } ProductInfo;
    ProductInfo *products = calloc(item_count, sizeof(ProductInfo));

    if (!products) {
        free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Memory allocation failed");
        return;
    }

    for (int i = 0; i < item_count; i++) {
        char id_buf[16];
        snprintf(id_buf, sizeof(id_buf), "%d", product_ids[i]);

        const char *p_query =
            "SELECT id, name, sku, price, stock_quantity, "
            "(SELECT url FROM product_images WHERE product_id = p.id AND is_primary = true LIMIT 1) AS image_url "
            "FROM products p WHERE id = $1 AND is_active = true";
        const char *p_params[] = { id_buf };
        PGresult *p_result = db_exec_params(conn, p_query, 1, p_params);

        if (!db_result_ok(p_result) || !db_result_has_rows(p_result)) {
            PQclear(p_result);
            int failed_id = product_ids[i];
            free(products); free(product_ids); free(quantities);
            free(c_email); free(c_name); free(c_phone); free(c_street);
            free(c_street_2); free(c_city); free(c_state); free(c_postal);
            free(c_country); free(c_notes);
            db_pool_release(pool, conn);
            char err[64];
            snprintf(err, sizeof(err), "Product %d not found or unavailable", failed_id);
            http_response_error(resp, HTTP_STATUS_BAD_REQUEST, err);
            return;
        }

        DbRow p_row = { .result = p_result, .row = 0 };
        products[i].id    = db_row_get_int(&p_row, "id");
        products[i].price = db_row_get_double(&p_row, "price");
        products[i].stock = db_row_get_int(&p_row, "stock_quantity");

        const char *pn = db_row_get_string(&p_row, "name");
        const char *ps = db_row_get_string(&p_row, "sku");
        const char *pi = db_row_get_string(&p_row, "image_url");
        if (pn) strncpy(products[i].name, pn, sizeof(products[i].name) - 1);
        if (ps) strncpy(products[i].sku,  ps, sizeof(products[i].sku)  - 1);
        if (pi) strncpy(products[i].image,pi, sizeof(products[i].image) - 1);
        PQclear(p_result);

        if (quantities[i] > products[i].stock) {
            char err[320];
            snprintf(err, sizeof(err), "Insufficient stock for %.200s (available: %d)",
                     products[i].name, products[i].stock);
            free(products); free(product_ids); free(quantities);
            free(c_email); free(c_name); free(c_phone); free(c_street);
            free(c_street_2); free(c_city); free(c_state); free(c_postal);
            free(c_country); free(c_notes);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_BAD_REQUEST, err);
            return;
        }

        subtotal += products[i].price * quantities[i];
    }

    /* Validate discount code if provided */
    double discount_amount = 0;
    char *validated_discount_code = NULL;

    if (discount_code != NULL && discount_code[0] != '\0') {
        const char *d_query =
            "SELECT discount_type, discount_value, minimum_order, maximum_discount, "
            "usage_limit, used_count "
            "FROM discount_codes "
            "WHERE code = $1 AND is_active = true "
            "AND (starts_at IS NULL OR starts_at <= NOW()) "
            "AND (expires_at IS NULL OR expires_at > NOW())";
        const char *d_params[] = { discount_code };
        PGresult *d_result = db_exec_params(conn, d_query, 1, d_params);

        if (db_result_ok(d_result) && db_result_has_rows(d_result)) {
            DbRow d_row = { .result = d_result, .row = 0 };
            const char *dtype   = db_row_get_string(&d_row, "discount_type");
            double dvalue       = db_row_get_double(&d_row, "discount_value");
            double dmin         = db_row_get_double(&d_row, "minimum_order");
            double dmax         = db_row_get_double(&d_row, "maximum_discount");
            int ulimit          = db_row_get_int(&d_row, "usage_limit");
            int ucount          = db_row_get_int(&d_row, "used_count");

            bool valid = (subtotal >= dmin) && (ulimit <= 0 || ucount < ulimit);
            if (valid) {
                discount_amount = (strcmp(dtype, "percentage") == 0)
                    ? subtotal * (dvalue / 100.0) : dvalue;
                if (dmax > 0 && discount_amount > dmax) discount_amount = dmax;
                validated_discount_code = str_dup(discount_code);
            }
        }
        PQclear(d_result);
    }

    double shipping_cost = 0;
    double tax_amount    = 0;
    double total = subtotal - discount_amount + shipping_cost + tax_amount;

    char *order_number = guest_generate_order_number();
    if (order_number == NULL) {
        free(products); free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes); free(validated_discount_code);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to generate order number");
        return;
    }

    if (!db_begin(conn)) {
        free(products); free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes); free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Transaction failed");
        return;
    }

    /* Insert order (user_id = NULL for guest) */
    const char *order_query =
        "INSERT INTO orders (order_number, user_id, guest_email, guest_name, payment_method, "
        "shipping_name, shipping_phone, shipping_street, shipping_street_2, "
        "shipping_city, shipping_state, shipping_postal_code, shipping_country, "
        "subtotal, shipping_cost, tax_amount, discount_amount, discount_code, total, notes) "
        "VALUES ($1, NULL, $2, $3, 'card', $4, $5, $6, $7, $8, $9, $10, $11, "
        "$12, $13, $14, $15, $16, $17, $18) "
        "RETURNING id";

    char subtotal_s[32], ship_s[32], tax_s[32], disc_s[32], total_s[32];
    snprintf(subtotal_s, sizeof(subtotal_s), "%.2f", subtotal);
    snprintf(ship_s, sizeof(ship_s), "%.2f", shipping_cost);
    snprintf(tax_s, sizeof(tax_s), "%.2f", tax_amount);
    snprintf(disc_s, sizeof(disc_s), "%.2f", discount_amount);
    snprintf(total_s, sizeof(total_s), "%.2f", total);

    const char *order_params[] = {
        order_number, c_email, c_name, c_name, c_phone,
        c_street, c_street_2, c_city, c_state, c_postal, c_country,
        subtotal_s, ship_s, tax_s, disc_s, validated_discount_code,
        total_s, c_notes
    };

    PGresult *order_result = db_exec_params(conn, order_query, 18, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_rollback(conn);
        free(products); free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes); free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create order");
        return;
    }

    const char *order_id_str = db_result_value(order_result);
    char *order_id = str_dup(order_id_str);
    PQclear(order_result);

    /* Insert order items and deduct stock */
    for (int i = 0; i < item_count; i++) {
        char pid_s[16], qty_s[16], price_s[32], line_s[32];
        double line = products[i].price * quantities[i];
        snprintf(pid_s,   sizeof(pid_s),   "%d", products[i].id);
        snprintf(qty_s,   sizeof(qty_s),   "%d", quantities[i]);
        snprintf(price_s, sizeof(price_s), "%.2f", products[i].price);
        snprintf(line_s,  sizeof(line_s),  "%.2f", line);

        const char *item_query =
            "INSERT INTO order_items (order_id, product_id, product_name, product_sku, "
            "product_image, quantity, unit_price, total_price) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)";
        const char *item_params[] = {
            order_id, pid_s, products[i].name, products[i].sku,
            products[i].image, qty_s, price_s, line_s
        };
        PGresult *item_res = db_exec_params(conn, item_query, 8, item_params);
        if (!db_result_ok(item_res)) {
            PQclear(item_res);
            db_rollback(conn);
            free(order_id); free(products); free(product_ids); free(quantities);
            free(c_email); free(c_name); free(c_phone); free(c_street);
            free(c_street_2); free(c_city); free(c_state); free(c_postal);
            free(c_country); free(c_notes); free(validated_discount_code);
            free(order_number);
            db_pool_release(pool, conn);
            http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create order items");
            return;
        }
        PQclear(item_res);

        const char *stock_query =
            "UPDATE products SET stock_quantity = stock_quantity - $1 WHERE id = $2";
        const char *stock_params[] = { qty_s, pid_s };
        PGresult *stock_res = db_exec_params(conn, stock_query, 2, stock_params);
        PQclear(stock_res);
    }

    /* Order history entry */
    const char *hist_query =
        "INSERT INTO order_history (order_id, status, notes) VALUES ($1, 'pending', 'Guest order created')";
    const char *hist_params[] = { order_id };
    PGresult *hist_res = db_exec_params(conn, hist_query, 1, hist_params);
    PQclear(hist_res);

    /* Update discount usage */
    if (validated_discount_code != NULL) {
        const char *du_query = "UPDATE discount_codes SET used_count = used_count + 1 WHERE code = $1";
        const char *du_params[] = { validated_discount_code };
        PGresult *du_res = db_exec_params(conn, du_query, 1, du_params);
        PQclear(du_res);
    }

    if (!db_commit(conn)) {
        db_rollback(conn);
        free(order_id); free(products); free(product_ids); free(quantities);
        free(c_email); free(c_name); free(c_phone); free(c_street);
        free(c_street_2); free(c_city); free(c_state); free(c_postal);
        free(c_country); free(c_notes); free(validated_discount_code);
        free(order_number);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to commit order");
        return;
    }

    db_pool_release(pool, conn);

    /* Send confirmation email */
    EmailOrderCreated email_order = {
        .order_number   = order_number,
        .customer_email = c_email,
        .customer_name  = c_name,
        .payment_method = "card",
        .shipping_city  = c_city,
        .shipping_state = c_state,
        .customer_notes = c_notes,
        .total          = total,
        .item_count     = item_count
    };
    if (email_service_send_order_confirmation(&email_order) != 0) {
        LOG_WARN("Guest order confirmation email failed for order %s", order_number);
    }
    if (email_service_send_internal_order_notification(&email_order) != 0) {
        LOG_WARN("Guest order internal notification failed for order %s", order_number);
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "id", order_id);
    cJSON_AddStringToObject(data, "order_number", order_number);
    cJSON_AddStringToObject(data, "status", "pending");
    cJSON_AddNumberToObject(data, "subtotal", subtotal);
    cJSON_AddNumberToObject(data, "discount_amount", discount_amount);
    cJSON_AddNumberToObject(data, "total", total);
    cJSON_AddNumberToObject(data, "item_count", item_count);

    LOG_INFO("Guest order created: %s for %s", order_number, c_email);

    free(order_id); free(products); free(product_ids); free(quantities);
    free(c_email); free(c_name); free(c_phone); free(c_street);
    free(c_street_2); free(c_city); free(c_state); free(c_postal);
    free(c_country); free(c_notes); free(validated_discount_code);
    free(order_number);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_CREATED, json);
    free(json);
}

/*
 * POST /api/guest-payments/create-intent
 *
 * Body:
 * {
 *   "order_id": "uuid",
 *   "guest_email": "customer@example.com"
 * }
 *
 * Creates a Stripe PaymentIntent for a guest order.
 * Verifies ownership via guest_email match.
 */
void handler_guest_payments_create_intent(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    if (!stripe_is_initialized()) {
        http_response_error(resp, HTTP_STATUS_SERVICE_UNAVAILABLE, "Payment service unavailable");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *order_id = json_get_string(body, "order_id", NULL);
    const char *guest_email = json_get_string(body, "guest_email", NULL);

    if (str_is_empty(order_id) || !uuid_validate(order_id) || str_is_empty(guest_email)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Valid order_id and guest_email required");
        return;
    }

    /* Copy strings before freeing body */
    char *c_order_id = str_dup(order_id);
    char *c_email = str_dup(guest_email);
    cJSON_Delete(body);

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        free(c_order_id);
        free(c_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get guest order — verify ownership via guest_email and ensure unpaid */
    const char *order_query =
        "SELECT id, order_number, total, payment_status, guest_email "
        "FROM orders "
        "WHERE id = $1 AND user_id IS NULL AND LOWER(guest_email) = LOWER($2)";
    const char *order_params[] = { c_order_id, c_email };

    PGresult *order_result = db_exec_params(conn, order_query, 2, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        free(c_order_id);
        free(c_email);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    DbRow row = { .result = order_result, .row = 0 };
    const char *order_number = db_row_get_string(&row, "order_number");
    double total = db_row_get_double(&row, "total");
    const char *payment_status = db_row_get_string(&row, "payment_status");

    if (strcmp(payment_status, "paid") == 0) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        free(c_order_id);
        free(c_email);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Order already paid");
        return;
    }

    /* Convert to cents */
    int64_t amount_cents = (int64_t)(total * 100);

    char *description = str_printf("Coldnb Order %s", order_number);

    /* Copy order_number before clearing result */
    PQclear(order_result);

    /* Create Payment Intent */
    StripePaymentIntent *pi = stripe_create_payment_intent(
        amount_cents, "brl", description, c_email, c_order_id
    );

    free(description);

    if (pi == NULL) {
        db_pool_release(pool, conn);
        free(c_order_id);
        free(c_email);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create payment");
        return;
    }

    /* Update order with payment intent ID */
    const char *update_query =
        "UPDATE orders SET payment_id = $1 WHERE id = $2";
    const char *update_params[] = { pi->id, c_order_id };
    PGresult *update_result = db_exec_params(conn, update_query, 2, update_params);
    PQclear(update_result);

    db_pool_release(pool, conn);

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "payment_intent_id", pi->id);
    cJSON_AddStringToObject(data, "client_secret", pi->client_secret);
    cJSON_AddStringToObject(data, "status", pi->status);
    cJSON_AddNumberToObject(data, "amount", (double)pi->amount);
    cJSON_AddStringToObject(data, "currency", pi->currency);

    stripe_payment_intent_free(pi);
    free(c_order_id);
    free(c_email);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Guest payment intent created for order");
}
