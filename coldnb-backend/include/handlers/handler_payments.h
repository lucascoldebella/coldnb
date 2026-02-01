#ifndef COLDNB_HANDLER_PAYMENTS_H
#define COLDNB_HANDLER_PAYMENTS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register payment routes */
void handler_payments_register(HttpRouter *router, DbPool *pool);

/* POST /api/payments/create-intent - Create Stripe PaymentIntent */
void handler_payments_create_intent(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/payments/pix - Create PIX payment (Brazil) */
void handler_payments_pix(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/webhooks/stripe - Handle Stripe webhooks */
void handler_payments_stripe_webhook(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_PAYMENTS_H */
