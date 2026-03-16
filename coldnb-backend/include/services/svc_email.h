#ifndef COLDNB_SVC_EMAIL_H
#define COLDNB_SVC_EMAIL_H

#include <stdbool.h>

typedef struct {
    const char *store_name;
    const char *site_url;
    const char *sender_email;
    const char *sender_name;
    const char *reply_to_email;
    const char *reply_to_name;
    const char *notification_email;
    const char *notification_name;
} EmailServiceConfig;

typedef struct {
    const char *submission_id;
    const char *name;
    const char *email;
    const char *phone;
    const char *subject;
    const char *message;
} EmailContactSubmission;

typedef struct {
    const char *order_number;
    const char *customer_email;
    const char *customer_name;
    const char *payment_method;
    const char *shipping_city;
    const char *shipping_state;
    const char *customer_notes;
    double total;
    int item_count;
} EmailOrderCreated;

typedef struct {
    const char *order_number;
    const char *customer_email;
    const char *customer_name;
    const char *status;
} EmailOrderStatusUpdate;

typedef struct {
    const char *order_number;
    const char *customer_email;
    const char *customer_name;
    const char *tracking_number;
    const char *carrier;
    const char *estimated_delivery;
} EmailOrderShipped;

int email_service_init(const EmailServiceConfig *config);
void email_service_shutdown(void);
bool email_service_is_initialized(void);

/* Generate an unsubscribe URL with HMAC token for the given email */
char *email_service_unsubscribe_url(const char *email);

/* Verify an unsubscribe token for the given email */
bool email_service_verify_unsubscribe_token(const char *email, const char *token);

typedef struct {
    const char *customer_email;
    const char *customer_name;
    int item_count;
    double cart_total;
    const char *top_product_name;
    const char *top_product_image;
} EmailAbandonedCart;

int email_service_send_contact_notification(const EmailContactSubmission *submission);
int email_service_send_order_confirmation(const EmailOrderCreated *order);
int email_service_send_internal_order_notification(const EmailOrderCreated *order);
int email_service_send_order_status_update(const EmailOrderStatusUpdate *update);
int email_service_send_order_shipped(const EmailOrderShipped *shipped);
int email_service_send_abandoned_cart(const EmailAbandonedCart *cart);

#endif /* COLDNB_SVC_EMAIL_H */
