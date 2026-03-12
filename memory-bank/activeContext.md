# Active Context — Coldnb

## Current Development Focus
**Sprint 2 — Order Operations & Tracking** (completed 2026-03-11)
Goal: operational readiness for order fulfillment post-launch.

## Sprint 2 Completed Tasks (2026-03-11)

### P0: Order Tracking Page ✅
- [x] Backend: `GET /api/orders/track?order_number=X&email=Y` (public, no auth)
- [x] Backend: Queries by order_number + email for privacy
- [x] Frontend: `OrderTrac.jsx` fully rewritten — form submits to API, shows visual status timeline (pending→confirmed→processing→shipped→delivered), tracking info (carrier, number, est. delivery), order items, order summary, status history
- [x] i18n: 30+ new keys in both pt.json + en.json (orderTracking.status.*, carrier, trackingInfo, etc.)
- [x] Carrier URL auto-link (Correios, DHL)

### P0: Admin Tracking Number Upload ✅
- [x] Backend: `PUT /api/admin/orders/:id/tracking` — sets tracking_number, carrier, estimated_delivery_date, auto-updates status to "shipped", auto-sets shipped_at, sends shipped email
- [x] Backend: `email_service_send_order_shipped()` — HTML email with carrier info, tracking link (Correios auto-URL), estimated delivery
- [x] Frontend: Admin order detail page (`/admin/orders/[id]`) — new "Tracking" card with carrier dropdown, tracking number input, est. delivery date picker, "Save & Notify" button
- [x] DB migration 006: Added `tracking_number`, `carrier`, `estimated_delivery_date` to orders table

### P0: Admin Inventory Management ✅
- [x] New page: `/admin/inventory` — full inventory dashboard
- [x] Stats cards: total products, out of stock (red), low stock ≤10 (orange), total units, inventory value
- [x] Filters: stock level (all/out/low/in), category, search by name/SKU, sort options
- [x] Inline stock editing: click "Edit Stock" → type new value → Save
- [x] Added "Inventory" nav item to admin sidebar with box icon

### P1: Customer Detail Page ✅
- [x] New page: `/admin/customers/[id]` — profile card (avatar/initials, name, email, phone, active/verified badges), customer stats (orders, total spent, avg order, member since), order history table with links to order detail

### P1: Product CRUD ✅ (already built, verified)
- All CRUD operations working: create, edit, delete, image management, categories

### P2: Shipping Config ✅ (already built)
- Full CRUD at `/admin/shipping` with distance-based zones

## Sprint 1 Completed (2026-03-10)
- [x] Stripe checkout (PaymentElement with card + PIX)
- [x] Cart merge on login
- [x] Cookie consent
- [x] Newsletter + contact forms wired to backend
- [x] Product data API-driven

## Current Platform State
- **Frontend:** Live at https://coldnb.com (PM2, port 3000)
- **Backend:** Live on VPS (systemd, port 8080 loopback)
- **Email:** Brevo transactional live (contact, order confirmation, status updates, **shipped**)
- **Stripe:** Backend handlers complete (card + PIX); frontend checkout wired; VPS config pending
- **Analytics:** Backend tracking active; admin analytics page live

## VPS Deployment Checklist (Sprint 1 + 2 Go-Live)
1. [ ] **DB migration 006:** Apply `sql/006_order_tracking.sql` on VPS PostgreSQL
2. [ ] **Stripe publishable key:** Add `NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_live_...` to VPS `.env.local`
3. [ ] **Stripe secret key:** Write to VPS `config/secrets/stripe_secret_key`
4. [ ] **Stripe webhook secret:** Write to VPS `config/secrets/stripe_webhook_secret`
5. [ ] **VPS server.conf:** Uncomment Stripe lines + set `stripe.pix_enabled=true`
6. [ ] **Stripe Dashboard:** Enable PIX, add webhook endpoint
7. [ ] **Nginx:** Ensure `/api/webhooks/stripe` proxied to backend :8080
8. [ ] **Deploy:** Run `./scripts/deploy.sh`
9. [ ] **Verify:** Test checkout with Stripe test card, verify webhook + tracking flow

## Known Open Issues
- `data/products.js` still used for non-production pages (cleanup in Sprint 3)
- Google Maps shows New York placeholder coordinates
- Placeholder contact info in `Footer1.jsx` (themesflat email, fake phone)
- 17 unused homepage themes and 24 unused product detail variants inflate bundle
- Privacy policy page (`/privacy-policy`) not yet created

## In-Progress / Do Not Interrupt
- Nothing currently in-progress

## Architecture Decisions (Preserved)
- Email split: Brevo API (backend business mail) vs Brevo SMTP via Supabase (auth mail) — permanent
- `brevo.sandbox_mode=true` for safe email testing
- Admin `/admin/email` page: operational reference only, future expansion point
- Order tracking is public (no auth) — uses order_number + email for privacy verification
