# Supabase + Brevo SMTP Setup

This project now separates email responsibilities in two paths:

- Backend transactional/project emails use the Brevo HTTP API from the C backend.
- Supabase Auth emails use Brevo SMTP inside the Supabase dashboard.

That split is intentional.

## Credential Types

Brevo uses two different credential types:

- `xkeysib-...`
  Use this as the Brevo API key for the backend secret file `brevo_api_key`.
- `xsmtpsib-...`
  Use this as the SMTP password in the Supabase dashboard.

Do not swap them.

## Backend Setup

Add your Brevo API v3 key to:

```bash
echo "xkeysib-REPLACE_ME" | sudo tee /etc/coldnb/secrets/brevo_api_key
sudo chmod 600 /etc/coldnb/secrets/brevo_api_key
```

Then configure these values in `/etc/coldnb/server.conf`:

```ini
brevo.api_key_file=/etc/coldnb/secrets/brevo_api_key
brevo.list_id=1
brevo.sandbox_mode=false

email.store_name=Coldnb
email.site_url=https://coldnb.com
email.sender_email=noreply@coldnb.com
email.sender_name=Coldnb
email.reply_to_email=support@coldnb.com
email.reply_to_name=Coldnb Support
email.notification_email=support@coldnb.com
email.notification_name=Coldnb Support
```

Notes:

- `email.sender_email` must be a verified sender in Brevo.
- `email.notification_email` is where internal contact/order alerts go.
- `brevo.sandbox_mode=true` is useful for testing request formatting without sending real emails.

## Supabase Panel Setup

As of March 8, 2026, the practical path in the Supabase dashboard is:

1. Open your project dashboard.
2. Go to `Authentication`.
3. Open the email/notifications area.
4. Find `SMTP Settings`.
5. Enable custom SMTP.
6. Fill the fields with your Brevo SMTP credentials:

   - Sender email: your verified Brevo sender, for example `noreply@coldnb.com`
   - Sender name: `Coldnb`
   - Host: `smtp-relay.brevo.com`
   - Port: `587`
   - Username: `a44c36001@smtp-brevo.com`
   - Password: your SMTP key `xsmtpsib-...`

7. Save the settings.
8. Keep the Supabase Site URL and redirect URLs correct for your auth flow:

   - `https://coldnb.com`
   - `https://coldnb.com/auth/callback`

## Before Testing

Make sure these are done in Brevo first:

- Verify the sender email or sending domain.
- If possible, authenticate the domain with SPF/DKIM for better deliverability.

## What The Backend Now Sends

The backend is prepared to send these through Brevo API:

- contact form notifications
- customer order confirmation emails
- internal new-order notifications
- customer order status update emails

Supabase remains responsible for:

- signup confirmation emails
- password reset emails
- magic link / auth emails
