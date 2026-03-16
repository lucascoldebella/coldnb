#ifndef COLDNB_HANDLER_GUEST_ORDERS_H
#define COLDNB_HANDLER_GUEST_ORDERS_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register guest order routes (public — no auth required) */
void handler_guest_orders_register(HttpRouter *router, DbPool *pool);

/* POST /api/guest-orders - Create an order as a guest (no account required) */
void handler_guest_orders_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/guest-payments/create-intent - Create Stripe PaymentIntent for guest order */
void handler_guest_payments_create_intent(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_GUEST_ORDERS_H */
