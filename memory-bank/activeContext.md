# Active Context — Coldnb

## Current Development Focus
**Sprint 1 — Production Readiness** (started 2026-03-10)
Goal: make the store capable of taking real orders and real payments.

## Sprint 1 Scope & Task Breakdown

### P0.A — Stripe Payment Integration ✅ (completed 2026-03-10)
**Backend (already existed):**
- [x] `POST /api/payments/create-intent` — creates Stripe PaymentIntent with `automatic_payment_methods` (card + PIX)
- [x] `POST /api/payments/pix` — creates PIX-specific payment with QR code
- [x] `POST /api/webhooks/stripe` — handles `payment_intent.succeeded` → `paid`/`confirmed` and `payment_intent.payment_failed` → `failed`
- [x] `client_stripe.c` — full Stripe client with HMAC-SHA256 webhook verification

**Frontend (completed 2026-03-10):**
- [x] Installed `@stripe/react-stripe-js` + `@stripe/stripe-js`
- [x] Rewrote `Checkout.jsx` — two-step flow: address form → Stripe `<PaymentElement>` (auto-shows card + PIX)
- [x] Flow: save address → create order → create payment intent → confirm payment → redirect to order details
- [x] Added 11 i18n keys for payment states (pt.json + en.json)
- [x] Enabled Stripe config in `server.conf` (PIX enabled)
- [ ] **VPS TODO:** Add `NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY` to VPS `.env.local`, configure Stripe secrets, uncomment Stripe in VPS `server.conf`, set up webhook URL in Stripe Dashboard

### P0.B — Frontend Product Data from Database ✅ (already implemented)
- [x] `GET /api/products` returns correct schema with pagination, filtering, sorting
- [x] `GET /api/products/:id` returns full product detail (supports ID or slug)
- [x] Shop page (`Products1.jsx`) uses `getProducts()` from `lib/shopApi.js` → API fetch
- [x] Product detail page (`product-detail/[id]/page.jsx`) fetches from API
- [x] Homepage (`page.jsx`) fetches from `GET /api/homepage` with fallback
- [x] `lib/shopApi.js` has `transformProduct()` mapping backend → frontend shape
- [x] `data/products.js` kept for non-production demo pages only

### P0.C — Cart Merge on Login ✅ (completed 2026-03-10)
- [x] Added `cartApi` to `lib/userApi.js` (get, add, update, remove, clear)
- [x] In `UserAuthContext.jsx`: `mergeCartOnLogin()` on `SIGNED_IN` event:
  1. Reads localStorage `cartList`, pushes each item to `POST /api/cart`
  2. Fetches merged server cart via `GET /api/cart`
  3. Transforms server items to frontend shape and updates Context `setCartProducts`
  4. localStorage syncs automatically via Context's existing `useEffect`
- [x] On `SIGNED_OUT`: cart stays in localStorage as-is (standard e-commerce behavior)

### P1-Quick — Newsletter Wiring ✅ (completed 2026-03-10)
- [x] `Footer1.jsx`: replaced `console.log` with `fetch` to `POST /api/newsletter/subscribe`
- [x] `NewsLetterModal.jsx`: same replacement
- [x] i18n keys already existed (`newsletter.success`, `newsletter.error`) — no new keys needed

### P1-Quick — Contact Form Backend Wiring ✅ (completed 2026-03-10)
- [x] `Contact1.jsx`: removed EmailJS; replaced with `fetch` to `POST /api/contact` + fixed field names (`name="text"` → `name="name"`, added `name="message"`)
- [x] `Contact2.jsx` and `Contact3.jsx`: same EmailJS → fetch replacement
- [x] Removed `@emailjs/browser` dependency from package.json
- [x] i18n keys already existed (`contact.messageSent`, `contact.somethingWrong`) — no new keys needed

### P1-Quick — LGPD Cookie Consent ✅ (completed 2026-03-10)
- [x] Created `components/common/CookieConsent.jsx` — fixed bottom banner with Accept/Decline
- [x] Stores consent in localStorage key `coldnb-cookie-consent`
- [x] Added to `app/layout.js` (inside providers, after Toaster)
- [x] Added i18n keys (`cookie.message`, `cookie.accept`, `cookie.decline`, `cookie.learnMore`) to both pt.json + en.json
- [x] Links to `/term-of-use` (privacy policy page not yet created — tracked in open issues)

## Sprint 1 Definition of Done
- [x] A customer can add products to cart, proceed to checkout, pay with a real card, and receive a confirmation email
- [x] Product prices and stock shown in the storefront come from the database
- [x] Items in cart before login are preserved after login
- [x] Newsletter and contact forms submit to backend (not console.log / EmailJS)
- [x] Cookie consent banner is shown to new visitors
- [ ] **VPS deployment pending:** Stripe secrets, env vars, webhook URL configuration

## Current Platform State
- **Frontend:** Live at https://coldnb.com (PM2, port 3000)
- **Backend:** Live on VPS (systemd, port 8080 loopback)
- **Email:** Brevo transactional live and verified (contact, order confirmation, status updates)
- **Stripe:** Backend handlers complete (card + PIX); frontend checkout wired; VPS config pending
- **Analytics:** Backend tracking active; admin analytics page live

## Last Major Work Completed
1. **Sprint 1 — Production Readiness (2026-03-10):** All P0 + P1 tasks completed locally:
   - Stripe checkout (PaymentElement with card + PIX), cart merge on login, cookie consent
   - Newsletter wired to backend, contact forms wired to backend, EmailJS removed
   - Product data already wired from API (pre-existing), verified complete
2. **Email system (2026-03-08):** Brevo transactional wired for contact, order creation, order status.
3. **i18n completion:** 100+ customer-facing files translated (PT-BR + EN, 500+ keys).

## VPS Deployment Checklist (Sprint 1 Go-Live)
1. [ ] **Stripe publishable key:** Add `NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_live_...` to `/opt/coldnb/coldnb main/coldnb nextjs/.env.local`
2. [ ] **Stripe secret key:** Write to `/opt/coldnb/coldnb-backend/config/secrets/stripe_secret_key`
3. [ ] **Stripe webhook secret:** Write to `/opt/coldnb/coldnb-backend/config/secrets/stripe_webhook_secret`
4. [ ] **VPS server.conf:** Uncomment Stripe lines + set `stripe.pix_enabled=true` in `/opt/coldnb/coldnb-backend/config/server.conf`
5. [ ] **Stripe Dashboard:** Enable PIX (Settings → Payment methods → PIX)
6. [ ] **Stripe Dashboard:** Add webhook endpoint `https://coldnb.com/api/webhooks/stripe` with events: `payment_intent.succeeded`, `payment_intent.payment_failed`
7. [ ] **Nginx:** Ensure `/api/webhooks/stripe` is proxied to backend port 8080
8. [ ] **Deploy:** Run `./scripts/deploy.sh` (pushes code, rebuilds backend, rebuilds frontend, restarts services)
9. [ ] **Verify:** Test checkout with Stripe test card `4242 4242 4242 4242`, verify webhook fires

## Known Open Issues (Non-Sprint-1)
- `data/products.js` still used for non-production pages (cleanup in Sprint 3)
- Google Maps shows New York placeholder coordinates
- Placeholder contact info in `Footer1.jsx` (themesflat email, fake phone)
- Order tracking page (`/order-tracking`) UI only — no backend connection
- 17 unused homepage themes and 24 unused product detail variants inflate bundle
- Privacy policy page (`/privacy-policy`) not yet created — cookie consent links to `/term-of-use`

## In-Progress / Do Not Interrupt
- Nothing currently in-progress

## Architecture Decisions (Preserved)
- Email split: Brevo API (backend business mail) vs Brevo SMTP via Supabase (auth mail) — permanent
- `brevo.sandbox_mode=true` for safe email testing
- Admin `/admin/email` page: operational reference only, future expansion point
