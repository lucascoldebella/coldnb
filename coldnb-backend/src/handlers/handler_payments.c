#include "handlers/handler_payments.h"
#include "clients/client_stripe.h"
#include "auth/auth_middleware.h"
#include "db/db_query.h"
#include "util/json_util.h"
#include "util/string_util.h"
#include "util/uuid_util.h"
#include "log/log.h"

#include <stdlib.h>
#include <string.h>

void handler_payments_register(HttpRouter *router, DbPool *pool) {
    /* Payment creation requires authentication */
    ROUTE_POST(router, "/api/payments/create-intent", handler_payments_create_intent, pool);
    ROUTE_POST(router, "/api/payments/pix", handler_payments_pix, pool);

    /* Webhook is public (verified via signature) */
    ROUTE_POST(router, "/api/webhooks/stripe", handler_payments_stripe_webhook, pool);
}

void handler_payments_create_intent(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

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
    if (str_is_empty(order_id) || !uuid_validate(order_id)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Valid order_id required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get order (verify ownership and status) */
    const char *order_query =
        "SELECT o.id, o.order_number, o.total, o.payment_status, u.email "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *order_params[] = { order_id, user_id };

    PGresult *order_result = db_exec_params(conn, order_query, 2, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    DbRow row = { .result = order_result, .row = 0 };
    const char *order_number = db_row_get_string(&row, "order_number");
    double total = db_row_get_double(&row, "total");
    const char *payment_status = db_row_get_string(&row, "payment_status");
    const char *customer_email = db_row_get_string(&row, "email");

    /* Check payment status */
    if (strcmp(payment_status, "paid") == 0) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Order already paid");
        return;
    }

    /* Convert to cents */
    int64_t amount_cents = (int64_t)(total * 100);

    /* Create description */
    char *description = str_printf("Coldnb Order %s", order_number);

    /* Copy values before clearing result */
    char *order_id_copy = str_dup(order_id);
    char *email_copy = str_dup(customer_email);
    PQclear(order_result);
    cJSON_Delete(body);

    /* Create Payment Intent */
    StripePaymentIntent *pi = stripe_create_payment_intent(
        amount_cents, "brl", description, email_copy, order_id_copy
    );

    free(description);
    free(email_copy);

    if (pi == NULL) {
        free(order_id_copy);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create payment");
        return;
    }

    /* Update order with payment intent ID */
    const char *update_query =
        "UPDATE orders SET payment_id = $1 WHERE id = $2";
    const char *update_params[] = { pi->id, order_id_copy };
    PGresult *update_result = db_exec_params(conn, update_query, 2, update_params);
    PQclear(update_result);

    free(order_id_copy);
    db_pool_release(pool, conn);

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "payment_intent_id", pi->id);
    cJSON_AddStringToObject(data, "client_secret", pi->client_secret);
    cJSON_AddStringToObject(data, "status", pi->status);
    cJSON_AddNumberToObject(data, "amount", (double)pi->amount);
    cJSON_AddStringToObject(data, "currency", pi->currency);

    stripe_payment_intent_free(pi);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("Payment intent created for order: %s", order_number);
}

void handler_payments_pix(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;
    const char *user_id = auth_get_user_id(req);

    if (user_id == NULL) {
        http_response_error(resp, HTTP_STATUS_UNAUTHORIZED, "Authentication required");
        return;
    }

    if (!stripe_pix_is_enabled()) {
        http_response_error(resp, HTTP_STATUS_SERVICE_UNAVAILABLE, "PIX payments not available");
        return;
    }

    cJSON *body = json_parse(req->body);
    if (body == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid JSON");
        return;
    }

    const char *order_id = json_get_string(body, "order_id", NULL);
    if (str_is_empty(order_id) || !uuid_validate(order_id)) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Valid order_id required");
        return;
    }

    PGconn *conn = db_pool_acquire(pool);
    if (conn == NULL) {
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Database connection failed");
        return;
    }

    /* Get order (verify ownership and status) */
    const char *order_query =
        "SELECT o.id, o.order_number, o.total, o.payment_status, u.email "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.id = $1 AND (u.id = $2 OR u.supabase_id = $2)";
    const char *order_params[] = { order_id, user_id };

    PGresult *order_result = db_exec_params(conn, order_query, 2, order_params);

    if (!db_result_ok(order_result) || !db_result_has_rows(order_result)) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_NOT_FOUND, "Order not found");
        return;
    }

    DbRow row = { .result = order_result, .row = 0 };
    const char *order_number = db_row_get_string(&row, "order_number");
    double total = db_row_get_double(&row, "total");
    const char *payment_status = db_row_get_string(&row, "payment_status");
    const char *customer_email = db_row_get_string(&row, "email");

    if (strcmp(payment_status, "paid") == 0) {
        PQclear(order_result);
        db_pool_release(pool, conn);
        cJSON_Delete(body);
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Order already paid");
        return;
    }

    int64_t amount_cents = (int64_t)(total * 100);
    char *description = str_printf("Coldnb Order %s", order_number);
    char *order_id_copy = str_dup(order_id);
    char *email_copy = str_dup(customer_email);
    PQclear(order_result);
    cJSON_Delete(body);

    /* Create PIX payment */
    StripePixPayment *pix = stripe_create_pix_payment(
        amount_cents, description, email_copy, order_id_copy
    );

    free(description);
    free(email_copy);

    if (pix == NULL) {
        free(order_id_copy);
        db_pool_release(pool, conn);
        http_response_error(resp, HTTP_STATUS_INTERNAL_ERROR, "Failed to create PIX payment");
        return;
    }

    /* Update order with payment ID and method */
    const char *update_query =
        "UPDATE orders SET payment_id = $1, payment_method = 'pix' WHERE id = $2";
    const char *update_params[] = { pix->id, order_id_copy };
    PGresult *update_result = db_exec_params(conn, update_query, 2, update_params);
    PQclear(update_result);

    free(order_id_copy);
    db_pool_release(pool, conn);

    /* Build response */
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "payment_id", pix->id);
    json_add_string_if(data, "qr_code", pix->qr_code);
    json_add_string_if(data, "qr_code_url", pix->qr_code_url);
    json_add_string_if(data, "expires_at", pix->expires_at);

    stripe_pix_payment_free(pix);

    cJSON *response = json_create_success(data);
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);

    http_response_json(resp, HTTP_STATUS_OK, json);
    free(json);

    LOG_INFO("PIX payment created for order: %s", order_number);
}

void handler_payments_stripe_webhook(HttpRequest *req, HttpResponse *resp, void *user_data) {
    DbPool *pool = (DbPool *)user_data;

    const char *signature = http_request_get_header(req, "Stripe-Signature");
    if (signature == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Missing signature");
        return;
    }

    /* Verify webhook signature */
    StripeWebhookEvent *event = stripe_verify_webhook(req->body, signature);
    if (event == NULL) {
        http_response_error(resp, HTTP_STATUS_BAD_REQUEST, "Invalid signature");
        return;
    }

    LOG_INFO("Stripe webhook received: %s", event->type);

    /* Handle payment_intent.succeeded */
    if (strcmp(event->type, "payment_intent.succeeded") == 0) {
        cJSON *data = cJSON_Parse(event->data);
        if (data != NULL) {
            cJSON *object = json_get_object(data, "object");
            if (object != NULL) {
                cJSON *metadata = json_get_object(object, "metadata");
                const char *order_id = metadata ?
                                       json_get_string(metadata, "order_id", NULL) : NULL;

                if (order_id != NULL && uuid_validate(order_id)) {
                    PGconn *conn = db_pool_acquire(pool);
                    if (conn != NULL) {
                        if (!db_begin(conn)) {
                            db_pool_release(pool, conn);
                        } else {
                            /* Update order status */
                            const char *update_query =
                                "UPDATE orders SET payment_status = 'paid', "
                                "status = 'confirmed', paid_at = NOW() "
                                "WHERE id = $1 AND payment_status != 'paid' "
                                "RETURNING order_number";
                            const char *update_params[] = { order_id };

                            PGresult *result = db_exec_params(conn, update_query, 1, update_params);

                            if (db_result_ok(result) && db_result_has_rows(result)) {
                                const char *order_number = db_result_value(result);
                                LOG_INFO("Order paid: %s", order_number);

                                /* Add history entry */
                                const char *history_query =
                                    "INSERT INTO order_history (order_id, status, notes) "
                                    "VALUES ($1, 'confirmed', 'Payment received via Stripe')";
                                const char *history_params[] = { order_id };
                                PGresult *history_result = db_exec_params(conn, history_query,
                                                                          1, history_params);
                                PQclear(history_result);

                                db_commit(conn);
                            } else {
                                db_rollback(conn);
                            }

                            PQclear(result);
                            db_pool_release(pool, conn);
                        }
                    }
                }
            }
            cJSON_Delete(data);
        }
    }

    /* Handle payment_intent.payment_failed */
    else if (strcmp(event->type, "payment_intent.payment_failed") == 0) {
        cJSON *data = cJSON_Parse(event->data);
        if (data != NULL) {
            cJSON *object = json_get_object(data, "object");
            if (object != NULL) {
                cJSON *metadata = json_get_object(object, "metadata");
                const char *order_id = metadata ?
                                       json_get_string(metadata, "order_id", NULL) : NULL;

                if (order_id != NULL && uuid_validate(order_id)) {
                    PGconn *conn = db_pool_acquire(pool);
                    if (conn != NULL) {
                        const char *update_query =
                            "UPDATE orders SET payment_status = 'failed' WHERE id = $1";
                        const char *update_params[] = { order_id };
                        PGresult *result = db_exec_params(conn, update_query, 1, update_params);
                        PQclear(result);

                        /* Add history entry */
                        const char *history_query =
                            "INSERT INTO order_history (order_id, status, notes) "
                            "VALUES ($1, 'payment_failed', 'Payment failed')";
                        const char *history_params[] = { order_id };
                        PGresult *history_result = db_exec_params(conn, history_query,
                                                                  1, history_params);
                        PQclear(history_result);

                        db_pool_release(pool, conn);

                        LOG_WARN("Payment failed for order: %s", order_id);
                    }
                }
            }
            cJSON_Delete(data);
        }
    }

    stripe_webhook_event_free(event);

    /* Always return 200 to acknowledge receipt */
    http_response_json(resp, HTTP_STATUS_OK, "{\"received\":true}");
}
