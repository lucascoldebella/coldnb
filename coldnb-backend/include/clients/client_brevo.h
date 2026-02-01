#ifndef COLDNB_CLIENT_BREVO_H
#define COLDNB_CLIENT_BREVO_H

#include <stdbool.h>

/* Brevo (formerly Sendinblue) configuration */
typedef struct {
    const char *api_key;
    int list_id;            /* Newsletter list ID */
} BrevoConfig;

/* Contact information */
typedef struct {
    char *email;
    char *first_name;
    char *last_name;
    int list_id;
} BrevoContact;

/* Initialize Brevo client
 * Returns 0 on success, -1 on error */
int brevo_init(const BrevoConfig *config);

/* Shutdown Brevo client */
void brevo_shutdown(void);

/* Add contact to newsletter list
 * Returns 0 on success, -1 on error */
int brevo_add_contact(const char *email, const char *name);

/* Remove contact from newsletter list
 * Returns 0 on success, -1 on error */
int brevo_remove_contact(const char *email);

/* Update contact
 * Returns 0 on success, -1 on error */
int brevo_update_contact(const char *email, const char *name);

/* Check if contact exists
 * Returns true if contact exists in list */
bool brevo_contact_exists(const char *email);

/* Check if Brevo is initialized */
bool brevo_is_initialized(void);

/* Get default list ID */
int brevo_get_list_id(void);

#endif /* COLDNB_CLIENT_BREVO_H */
