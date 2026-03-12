#include "services/svc_email.h"
#include "clients/client_brevo.h"
#include "log/log.h"
#include "util/string_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
    char *store_name;
    char *site_url;
    char *sender_email;
    char *sender_name;
    char *reply_to_email;
    char *reply_to_name;
    char *notification_email;
    char *notification_name;
    bool initialized;
} email_state = {0};

static char *dup_or_default(const char *value, const char *fallback) {
    const char *selected = str_is_empty(value) ? fallback : value;
    return selected ? str_dup(selected) : NULL;
}

static char *normalize_multiline(const char *text) {
    char *step1 = str_replace(text ? text : "", "\r\n", "\n");
    if (step1 == NULL) {
        return NULL;
    }

    char *step2 = str_replace(step1, "\r", "\n");
    free(step1);
    return step2;
}

static char *html_multiline(const char *text) {
    char *normalized = normalize_multiline(text ? text : "");
    if (normalized == NULL) {
        return NULL;
    }

    char *escaped = str_escape_html(normalized);
    free(normalized);
    if (escaped == NULL) {
        return NULL;
    }

    char *html = str_replace(escaped, "\n", "<br/>");
    free(escaped);
    return html;
}

static char *humanize_status(const char *status) {
    if (str_is_empty(status)) {
        return str_dup("Updated");
    }

    char *copy = str_dup(status);
    if (copy == NULL) {
        return NULL;
    }

    bool new_word = true;
    for (size_t i = 0; copy[i] != '\0'; i++) {
        if (copy[i] == '_' || copy[i] == '-') {
            copy[i] = ' ';
            new_word = true;
            continue;
        }

        if (new_word && copy[i] >= 'a' && copy[i] <= 'z') {
            copy[i] = (char)(copy[i] - ('a' - 'A'));
        }
        new_word = (copy[i] == ' ');
    }

    return copy;
}

static char *orders_url(void) {
    return str_printf("%s/my-account-orders", email_state.site_url);
}

static int email_send(const BrevoEmailRequest *request) {
    char *message_id = NULL;
    int result = brevo_send_email(request, &message_id);
    free(message_id);
    return result;
}

int email_service_init(const EmailServiceConfig *config) {
    email_service_shutdown();

    if (config == NULL) {
        LOG_WARN("Email service config missing, transactional email disabled");
        return 0;
    }

    if (!brevo_is_initialized()) {
        LOG_WARN("Brevo client unavailable, transactional email disabled");
        return 0;
    }

    email_state.store_name = dup_or_default(config->store_name, "Coldnb");
    email_state.site_url = dup_or_default(config->site_url, "https://coldnb.com");
    email_state.sender_email = dup_or_default(config->sender_email, NULL);
    email_state.sender_name = dup_or_default(config->sender_name,
                                             config->store_name ? config->store_name : "Coldnb");
    email_state.reply_to_email = dup_or_default(config->reply_to_email, config->sender_email);
    email_state.reply_to_name = dup_or_default(config->reply_to_name,
                                               config->sender_name ? config->sender_name :
                                               (config->store_name ? config->store_name : "Coldnb"));
    email_state.notification_email = dup_or_default(config->notification_email, NULL);
    email_state.notification_name = dup_or_default(config->notification_name,
                                                   config->store_name ? config->store_name : "Coldnb");

    if (email_state.store_name == NULL ||
        email_state.site_url == NULL ||
        email_state.sender_email == NULL ||
        email_state.sender_name == NULL) {
        LOG_WARN("Email service incomplete, transactional email disabled");
        email_service_shutdown();
        return 0;
    }

    if (email_state.reply_to_email == NULL) {
        email_state.reply_to_email = str_dup(email_state.sender_email);
    }
    if (email_state.reply_to_name == NULL) {
        email_state.reply_to_name = str_dup(email_state.sender_name);
    }

    email_state.initialized = true;

    LOG_INFO("Email service initialized%s",
             email_state.notification_email ? "" : " (internal notifications disabled)");
    return 0;
}

void email_service_shutdown(void) {
    free(email_state.store_name);
    free(email_state.site_url);
    free(email_state.sender_email);
    free(email_state.sender_name);
    free(email_state.reply_to_email);
    free(email_state.reply_to_name);
    free(email_state.notification_email);
    free(email_state.notification_name);
    memset(&email_state, 0, sizeof(email_state));
}

bool email_service_is_initialized(void) {
    return email_state.initialized;
}

int email_service_send_contact_notification(const EmailContactSubmission *submission) {
    if (!email_state.initialized || submission == NULL || str_is_empty(email_state.notification_email)) {
        return -1;
    }

    char *safe_name = str_escape_html(submission->name ? submission->name : "");
    char *safe_email = str_escape_html(submission->email ? submission->email : "");
    char *safe_phone = str_escape_html(submission->phone ? submission->phone : "");
    char *safe_subject = str_escape_html(submission->subject ? submission->subject : "No subject");
    char *safe_message = html_multiline(submission->message ? submission->message : "");
    char *subject = str_printf("New contact form submission from %s",
                               !str_is_empty(submission->name) ? submission->name : "Website visitor");
    char *text_body = str_printf(
        "New contact submission\n\n"
        "ID: %s\n"
        "Name: %s\n"
        "Email: %s\n"
        "Phone: %s\n"
        "Subject: %s\n\n"
        "Message:\n%s\n",
        submission->submission_id ? submission->submission_id : "-",
        submission->name ? submission->name : "-",
        submission->email ? submission->email : "-",
        !str_is_empty(submission->phone) ? submission->phone : "-",
        submission->subject ? submission->subject : "No subject",
        submission->message ? submission->message : "");
    char *html_body = str_printf(
        "<html><body style=\"font-family:Arial,sans-serif;color:#1f2937;line-height:1.6\">"
        "<h2 style=\"margin:0 0 16px\">New contact form submission</h2>"
        "<p><strong>ID:</strong> %s</p>"
        "<p><strong>Name:</strong> %s</p>"
        "<p><strong>Email:</strong> %s</p>"
        "<p><strong>Phone:</strong> %s</p>"
        "<p><strong>Subject:</strong> %s</p>"
        "<p><strong>Message:</strong><br/>%s</p>"
        "</body></html>",
        submission->submission_id ? submission->submission_id : "-",
        safe_name ? safe_name : "-",
        safe_email ? safe_email : "-",
        !str_is_empty(submission->phone) ? (safe_phone ? safe_phone : "-") : "-",
        safe_subject ? safe_subject : "No subject",
        safe_message ? safe_message : "");

    BrevoEmailRequest request = {
        .sender_email = email_state.sender_email,
        .sender_name = email_state.sender_name,
        .reply_to_email = !str_is_empty(submission->email) ? submission->email : email_state.reply_to_email,
        .reply_to_name = !str_is_empty(submission->name) ? submission->name : email_state.reply_to_name,
        .to_email = email_state.notification_email,
        .to_name = email_state.notification_name,
        .subject = subject,
        .html_content = html_body,
        .text_content = text_body,
        .tag = "contact-form"
    };

    int result = email_send(&request);

    free(safe_name);
    free(safe_email);
    free(safe_phone);
    free(safe_subject);
    free(safe_message);
    free(subject);
    free(text_body);
    free(html_body);

    return result;
}

int email_service_send_order_confirmation(const EmailOrderCreated *order) {
    if (!email_state.initialized || order == NULL || str_is_empty(order->customer_email)) {
        return -1;
    }

    char total_str[32];
    snprintf(total_str, sizeof(total_str), "%.2f", order->total);

    char *orders_link = orders_url();
    char *safe_name = str_escape_html(order->customer_name ? order->customer_name : "Customer");
    char *safe_payment = str_escape_html(order->payment_method ? order->payment_method : "Pending confirmation");
    char *safe_city = str_escape_html(order->shipping_city ? order->shipping_city : "");
    char *safe_state = str_escape_html(order->shipping_state ? order->shipping_state : "");
    char *safe_notes = html_multiline(order->customer_notes ? order->customer_notes : "");
    char *notes_section = (!str_is_empty(order->customer_notes) && safe_notes != NULL)
        ? str_printf("<p><strong>Your notes:</strong><br/>%s</p>", safe_notes)
        : str_dup("");
    char *subject = str_printf("%s order confirmation %s",
                               email_state.store_name,
                               order->order_number ? order->order_number : "");
    char *text_body = str_printf(
        "Hello %s,\n\n"
        "We received your order %s.\n"
        "Total: R$ %s\n"
        "Items: %d\n"
        "Payment method: %s\n"
        "Shipping destination: %s%s%s\n\n"
        "You can review your orders here: %s\n\n"
        "Thank you,\n%s\n",
        order->customer_name ? order->customer_name : "Customer",
        order->order_number ? order->order_number : "-",
        total_str,
        order->item_count,
        order->payment_method ? order->payment_method : "Pending confirmation",
        !str_is_empty(order->shipping_city) ? order->shipping_city : "-",
        !str_is_empty(order->shipping_city) && !str_is_empty(order->shipping_state) ? ", " : "",
        !str_is_empty(order->shipping_state) ? order->shipping_state : "",
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);
    char *html_body = str_printf(
        "<html><body style=\"font-family:Arial,sans-serif;color:#1f2937;line-height:1.6\">"
        "<h2 style=\"margin:0 0 16px\">Order received</h2>"
        "<p>Hello %s,</p>"
        "<p>We received your order <strong>%s</strong>.</p>"
        "<ul>"
        "<li><strong>Total:</strong> R$ %s</li>"
        "<li><strong>Items:</strong> %d</li>"
        "<li><strong>Payment method:</strong> %s</li>"
        "<li><strong>Shipping destination:</strong> %s%s%s</li>"
        "</ul>"
        "%s"
        "<p><a href=\"%s\" style=\"display:inline-block;padding:10px 16px;background:#111827;color:#ffffff;text-decoration:none;border-radius:6px\">View my orders</a></p>"
        "<p>Thank you,<br/>%s</p>"
        "</body></html>",
        safe_name ? safe_name : "Customer",
        order->order_number ? order->order_number : "-",
        total_str,
        order->item_count,
        safe_payment ? safe_payment : "Pending confirmation",
        !str_is_empty(order->shipping_city) ? (safe_city ? safe_city : "-") : "-",
        !str_is_empty(order->shipping_city) && !str_is_empty(order->shipping_state) ? ", " : "",
        !str_is_empty(order->shipping_state) ? (safe_state ? safe_state : "") : "",
        notes_section ? notes_section : "",
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);

    BrevoEmailRequest request = {
        .sender_email = email_state.sender_email,
        .sender_name = email_state.sender_name,
        .reply_to_email = email_state.reply_to_email,
        .reply_to_name = email_state.reply_to_name,
        .to_email = order->customer_email,
        .to_name = order->customer_name,
        .subject = subject,
        .html_content = html_body,
        .text_content = text_body,
        .tag = "order-confirmation"
    };

    int result = email_send(&request);

    free(orders_link);
    free(safe_name);
    free(safe_payment);
    free(safe_city);
    free(safe_state);
    free(safe_notes);
    free(notes_section);
    free(subject);
    free(text_body);
    free(html_body);

    return result;
}

int email_service_send_internal_order_notification(const EmailOrderCreated *order) {
    if (!email_state.initialized || order == NULL || str_is_empty(email_state.notification_email)) {
        return -1;
    }

    char total_str[32];
    snprintf(total_str, sizeof(total_str), "%.2f", order->total);

    char *safe_name = str_escape_html(order->customer_name ? order->customer_name : "");
    char *safe_email = str_escape_html(order->customer_email ? order->customer_email : "");
    char *safe_payment = str_escape_html(order->payment_method ? order->payment_method : "");
    char *subject = str_printf("New order %s", order->order_number ? order->order_number : "");
    char *text_body = str_printf(
        "New order received\n\n"
        "Order: %s\n"
        "Customer: %s\n"
        "Email: %s\n"
        "Total: R$ %s\n"
        "Items: %d\n"
        "Payment method: %s\n",
        order->order_number ? order->order_number : "-",
        order->customer_name ? order->customer_name : "-",
        order->customer_email ? order->customer_email : "-",
        total_str,
        order->item_count,
        order->payment_method ? order->payment_method : "-");
    char *html_body = str_printf(
        "<html><body style=\"font-family:Arial,sans-serif;color:#1f2937;line-height:1.6\">"
        "<h2 style=\"margin:0 0 16px\">New order received</h2>"
        "<ul>"
        "<li><strong>Order:</strong> %s</li>"
        "<li><strong>Customer:</strong> %s</li>"
        "<li><strong>Email:</strong> %s</li>"
        "<li><strong>Total:</strong> R$ %s</li>"
        "<li><strong>Items:</strong> %d</li>"
        "<li><strong>Payment method:</strong> %s</li>"
        "</ul>"
        "</body></html>",
        order->order_number ? order->order_number : "-",
        safe_name ? safe_name : "-",
        safe_email ? safe_email : "-",
        total_str,
        order->item_count,
        safe_payment ? safe_payment : "-");

    BrevoEmailRequest request = {
        .sender_email = email_state.sender_email,
        .sender_name = email_state.sender_name,
        .reply_to_email = email_state.reply_to_email,
        .reply_to_name = email_state.reply_to_name,
        .to_email = email_state.notification_email,
        .to_name = email_state.notification_name,
        .subject = subject,
        .html_content = html_body,
        .text_content = text_body,
        .tag = "internal-order"
    };

    int result = email_send(&request);

    free(safe_name);
    free(safe_email);
    free(safe_payment);
    free(subject);
    free(text_body);
    free(html_body);

    return result;
}

int email_service_send_order_shipped(const EmailOrderShipped *shipped) {
    if (!email_state.initialized || shipped == NULL || str_is_empty(shipped->customer_email)) {
        return -1;
    }

    char *orders_link = orders_url();
    char *safe_name = str_escape_html(shipped->customer_name ? shipped->customer_name : "Customer");
    char *safe_tracking = str_escape_html(shipped->tracking_number ? shipped->tracking_number : "");
    char *safe_carrier = str_escape_html(shipped->carrier ? shipped->carrier : "");

    /* Build tracking URL based on carrier */
    char *tracking_link = NULL;
    if (!str_is_empty(shipped->tracking_number) && !str_is_empty(shipped->carrier)) {
        if (strcasecmp(shipped->carrier, "correios") == 0 ||
            strcasecmp(shipped->carrier, "sedex") == 0 ||
            strcasecmp(shipped->carrier, "pac") == 0) {
            tracking_link = str_printf(
                "https://www.linkcorreios.com.br/?id=%s", shipped->tracking_number);
        }
    }

    char *delivery_section = "";
    bool free_delivery = false;
    if (!str_is_empty(shipped->estimated_delivery)) {
        delivery_section = str_printf(
            "<li><strong>Estimated delivery:</strong> %s</li>",
            shipped->estimated_delivery);
        free_delivery = true;
    }

    char *tracking_button = "";
    bool free_tracking_button = false;
    if (tracking_link != NULL) {
        tracking_button = str_printf(
            "<p><a href=\"%s\" style=\"display:inline-block;padding:10px 16px;"
            "background:#111827;color:#ffffff;text-decoration:none;"
            "border-radius:6px\">Track my package</a></p>",
            tracking_link);
        free_tracking_button = true;
    }

    char *subject = str_printf("%s order shipped %s",
                               email_state.store_name,
                               shipped->order_number ? shipped->order_number : "");
    char *text_body = str_printf(
        "Hello %s,\n\n"
        "Your order %s has been shipped!\n"
        "Carrier: %s\n"
        "Tracking number: %s\n"
        "%s%s"
        "\nYou can review your orders here: %s\n\n"
        "%s\n",
        shipped->customer_name ? shipped->customer_name : "Customer",
        shipped->order_number ? shipped->order_number : "-",
        shipped->carrier ? shipped->carrier : "-",
        shipped->tracking_number ? shipped->tracking_number : "-",
        !str_is_empty(shipped->estimated_delivery) ? "Estimated delivery: " : "",
        !str_is_empty(shipped->estimated_delivery) ? shipped->estimated_delivery : "",
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);
    char *html_body = str_printf(
        "<html><body style=\"font-family:Arial,sans-serif;color:#1f2937;line-height:1.6\">"
        "<h2 style=\"margin:0 0 16px\">Your order has been shipped!</h2>"
        "<p>Hello %s,</p>"
        "<p>Your order <strong>%s</strong> is on its way.</p>"
        "<ul>"
        "<li><strong>Carrier:</strong> %s</li>"
        "<li><strong>Tracking number:</strong> %s</li>"
        "%s"
        "</ul>"
        "%s"
        "<p><a href=\"%s\" style=\"display:inline-block;padding:10px 16px;"
        "background:#6b7280;color:#ffffff;text-decoration:none;"
        "border-radius:6px;margin-top:8px\">View my orders</a></p>"
        "<p>%s</p>"
        "</body></html>",
        safe_name ? safe_name : "Customer",
        shipped->order_number ? shipped->order_number : "-",
        !str_is_empty(safe_carrier) ? safe_carrier : "-",
        !str_is_empty(safe_tracking) ? safe_tracking : "-",
        delivery_section,
        tracking_button,
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);

    BrevoEmailRequest request = {
        .sender_email = email_state.sender_email,
        .sender_name = email_state.sender_name,
        .reply_to_email = email_state.reply_to_email,
        .reply_to_name = email_state.reply_to_name,
        .to_email = shipped->customer_email,
        .to_name = shipped->customer_name,
        .subject = subject,
        .html_content = html_body,
        .text_content = text_body,
        .tag = "order-shipped"
    };

    int result = email_send(&request);

    free(orders_link);
    free(safe_name);
    free(safe_tracking);
    free(safe_carrier);
    free(tracking_link);
    if (free_delivery) free(delivery_section);
    if (free_tracking_button) free(tracking_button);
    free(subject);
    free(text_body);
    free(html_body);

    return result;
}

int email_service_send_order_status_update(const EmailOrderStatusUpdate *update) {
    if (!email_state.initialized || update == NULL || str_is_empty(update->customer_email)) {
        return -1;
    }

    char *orders_link = orders_url();
    char *safe_name = str_escape_html(update->customer_name ? update->customer_name : "Customer");
    char *human_status_text = humanize_status(update->status);
    char *safe_status = str_escape_html(human_status_text ? human_status_text : update->status);
    char *subject = str_printf("%s order update %s",
                               email_state.store_name,
                               update->order_number ? update->order_number : "");
    char *text_body = str_printf(
        "Hello %s,\n\n"
        "Your order %s is now %s.\n"
        "You can review your orders here: %s\n\n"
        "%s\n",
        update->customer_name ? update->customer_name : "Customer",
        update->order_number ? update->order_number : "-",
        human_status_text ? human_status_text : "Updated",
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);
    char *html_body = str_printf(
        "<html><body style=\"font-family:Arial,sans-serif;color:#1f2937;line-height:1.6\">"
        "<h2 style=\"margin:0 0 16px\">Order status updated</h2>"
        "<p>Hello %s,</p>"
        "<p>Your order <strong>%s</strong> is now <strong>%s</strong>.</p>"
        "<p><a href=\"%s\" style=\"display:inline-block;padding:10px 16px;background:#111827;color:#ffffff;text-decoration:none;border-radius:6px\">Open my orders</a></p>"
        "<p>%s</p>"
        "</body></html>",
        safe_name ? safe_name : "Customer",
        update->order_number ? update->order_number : "-",
        safe_status ? safe_status : "Updated",
        orders_link ? orders_link : email_state.site_url,
        email_state.store_name);

    BrevoEmailRequest request = {
        .sender_email = email_state.sender_email,
        .sender_name = email_state.sender_name,
        .reply_to_email = email_state.reply_to_email,
        .reply_to_name = email_state.reply_to_name,
        .to_email = update->customer_email,
        .to_name = update->customer_name,
        .subject = subject,
        .html_content = html_body,
        .text_content = text_body,
        .tag = "order-status"
    };

    int result = email_send(&request);

    free(orders_link);
    free(safe_name);
    free(human_status_text);
    free(safe_status);
    free(subject);
    free(text_body);
    free(html_body);

    return result;
}
