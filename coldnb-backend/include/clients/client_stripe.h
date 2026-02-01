#ifndef COLDNB_CLIENT_STRIPE_H
#define COLDNB_CLIENT_STRIPE_H

#include <stdbool.h>
#include <stdint.h>

/* Stripe configuration */
typedef struct {
    const char *secret_key;
    const char *webhook_secret;
    bool pix_enabled;
} StripeConfig;

/* Payment Intent result */
typedef struct {
    char *id;               /* Payment Intent ID (pi_xxx) */
    char *client_secret;    /* Client secret for frontend */
    char *status;           /* requires_payment_method, requires_confirmation, etc. */
    int64_t amount;         /* Amount in cents */
    char *currency;
} StripePaymentIntent;

/* PIX payment result */
typedef struct {
    char *id;               /* Payment Intent ID */
    char *qr_code;          /* PIX QR code string */
    char *qr_code_url;      /* URL to QR code image */
    char *expires_at;       /* Expiration timestamp */
} StripePixPayment;

/* Webhook event */
typedef struct {
    char *id;               /* Event ID */
    char *type;             /* Event type (payment_intent.succeeded, etc.) */
    char *data;             /* Event data as JSON string */
} StripeWebhookEvent;

/* Initialize Stripe client
 * Returns 0 on success, -1 on error */
int stripe_init(const StripeConfig *config);

/* Shutdown Stripe client */
void stripe_shutdown(void);

/* Create a Payment Intent
 * amount is in cents, currency is lowercase (e.g., "brl", "usd")
 * Returns StripePaymentIntent on success, NULL on failure
 * Caller must free with stripe_payment_intent_free */
StripePaymentIntent *stripe_create_payment_intent(int64_t amount, const char *currency,
                                                   const char *description,
                                                   const char *customer_email,
                                                   const char *metadata_order_id);

/* Create a PIX payment (Brazil)
 * Returns StripePixPayment on success, NULL on failure
 * Caller must free with stripe_pix_payment_free */
StripePixPayment *stripe_create_pix_payment(int64_t amount, const char *description,
                                            const char *customer_email,
                                            const char *metadata_order_id);

/* Retrieve a Payment Intent by ID
 * Returns StripePaymentIntent on success, NULL on failure */
StripePaymentIntent *stripe_get_payment_intent(const char *payment_intent_id);

/* Verify webhook signature and parse event
 * payload is the raw request body
 * signature is the Stripe-Signature header
 * Returns StripeWebhookEvent on success, NULL on failure
 * Caller must free with stripe_webhook_event_free */
StripeWebhookEvent *stripe_verify_webhook(const char *payload, const char *signature);

/* Free payment intent */
void stripe_payment_intent_free(StripePaymentIntent *pi);

/* Free PIX payment */
void stripe_pix_payment_free(StripePixPayment *pix);

/* Free webhook event */
void stripe_webhook_event_free(StripeWebhookEvent *event);

/* Check if Stripe is initialized */
bool stripe_is_initialized(void);

/* Check if PIX is enabled */
bool stripe_pix_is_enabled(void);

#endif /* COLDNB_CLIENT_STRIPE_H */
