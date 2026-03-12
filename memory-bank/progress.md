# Progress Ledger — Coldnb

## Fully Functional (Production-Ready) ✅
- ✅ Next.js 15 + React 19 frontend (App Router)
- ✅ C backend with libmicrohttpd (HTTP server, routing, request handling)
- ✅ PostgreSQL schema (all tables: users, products, categories, orders, cart, wishlist, etc.)
- ✅ Dual authentication: Supabase JWT (customers) + custom JWT (admins)
- ✅ Admin login (`POST /api/admin/login`, Argon2id password hashing)
- ✅ Admin dashboard UI (sidebar, header, navigation)
- ✅ Admin permission system (granular per-endpoint, super_admin bypass)
- ✅ Product listing pages (18 homepage themes, 7+ shop layouts)
- ✅ Product detail pages (25+ variants — one selected for production use)
- ✅ Customer-facing bilingual UI (PT-BR + EN, 500+ keys, 100+ components)
- ✅ Shopping cart (localStorage — frontend state management)
- ✅ Wishlist (localStorage — frontend state management)
- ✅ Server-side cart and wishlist (DB schema + backend handlers)
- ✅ Account pages (dashboard, addresses, orders, order details — UI complete)
- ✅ Order creation backend (`POST /api/orders`)
- ✅ Order status lifecycle (pending → confirmed → processing → shipped → delivered → cancelled)
- ✅ Admin order management (list, filter, update status)
- ✅ Transactional email: contact form notification (internal alert)
- ✅ Transactional email: order confirmation (customer + internal)
- ✅ Transactional email: order status update (customer notification)
- ✅ Email architecture split (Brevo API for backend, SMTP relay for Supabase auth)
- ✅ Admin email management page (`/admin/email`)
- ✅ Contact form backend (`POST /api/contact` → DB + email notification)
- ✅ Newsletter subscriber management (DB + Brevo sync)
- ✅ Image upload API (`POST /api/admin/upload` → `public/uploads/products/`)
- ✅ Analytics tracking (page views, product views — DB tables + backend middleware)
- ✅ Backend analytics handler (`/api/admin/analytics`)
- ✅ Admin analytics page (revenue, orders, top products charts)
- ✅ Category management (DB + backend handlers)
- ✅ Discount code schema (DB table with all fields)
- ✅ Shipping zones schema (SQL migration applied)
- ✅ Navigation menus schema (SQL migration applied)
- ✅ VPS deployment (Nginx + PM2 + systemd + UFW + CrowdSec + fail2ban)
- ✅ SSL (Let's Encrypt via Nginx)

## Partial / In Progress 🔄
- 🔄 **Admin: marketing/promotions:** Page exists; discount code UI CRUD completeness unknown
- 🔄 **Theme:** Multiple homepage themes built; only one is the production default (page.jsx redirects)
- 🔄 **Email templates:** Brevo sends emails but templates are plain/minimal — no rich HTML templates

## Recently Completed (Sprint 2 — 2026-03-11)
- ✅ **Order tracking page:** `/order-tracking` wired to `GET /api/orders/track` (public, order_number + email lookup). Visual status timeline, tracking info, items, history.
- ✅ **Admin tracking upload:** `PUT /api/admin/orders/:id/tracking` — sets tracking_number, carrier, estimated_delivery, auto-sets status to shipped, sends shipped email to customer.
- ✅ **Shipped email:** New `email_service_send_order_shipped()` with tracking link (Correios URL auto-generated), carrier info, estimated delivery.
- ✅ **DB migration 006:** Added `tracking_number`, `carrier`, `estimated_delivery_date` columns to orders table.
- ✅ **Admin inventory page:** `/admin/inventory` — stock levels with color badges, filters (out/low/in stock, category, search), inline stock editing, stats cards (total products, out of stock, low stock, total units, inventory value).
- ✅ **Admin customer detail:** `/admin/customers/[id]` — profile card, customer stats (orders, total spent, avg order), order history table.
- ✅ **Admin product CRUD:** Verified fully built (create/edit/delete/images/categories).
- ✅ **Admin shipping config:** Already built with full CRUD at `/admin/shipping`.

## Sprint 1 Completed (2026-03-10)
- ✅ **Stripe payments:** Full backend + frontend checkout with PaymentElement (card + PIX). VPS config pending.
- ✅ **Cart sync:** localStorage → server merge on login via `mergeCartOnLogin()` in UserAuthContext
- ✅ **Product data connection:** Shop/detail/homepage all API-driven via `lib/shopApi.js`
- ✅ **Contact form frontend:** `Contact1/2/3.jsx` → `POST /api/contact` (EmailJS removed)
- ✅ **Newsletter frontend:** Footer + modal → `POST /api/newsletter/subscribe`
- ✅ **LGPD cookie consent:** Banner with Accept/Decline, localStorage, bilingual
- ✅ **Stripe webhook:** `POST /api/webhooks/stripe` handles `payment_intent.succeeded` / `payment_intent.payment_failed`

## Planned / Not Started ⬜
- ⬜ **Guest checkout:** Schema supports `user_id NULL` on orders, but guest cart → checkout flow not built
- ⬜ **Shipping rate calculation:** Real-time carrier rate lookup (Correios integration)
- ⬜ **Return/refund workflow:** No frontend or backend support for returns
- ⬜ **Email preference management:** No unsubscribe flow beyond Brevo list management
- ⬜ **SMS/WhatsApp notifications:** Not started
- ⬜ **Loyalty/rewards program:** Not started
- ⬜ **Abandoned cart recovery:** Not started
- ⬜ **SEO meta tags/Open Graph:** Not implemented (no meta titles/descriptions on product pages)
- ⬜ **Sitemap generation:** Not implemented
- ⬜ **PWA / push notifications:** Not started
- ⬜ **Social login (OAuth):** Supabase supports it but not wired in frontend
- ⬜ **Password reset flow:** Supabase handles this; UI page exists but needs verification
- ⬜ **Image optimization pipeline:** No CDN; images served from `public/` directly
- ⬜ **Product search (full-text):** No search implemented
- ⬜ **Product recommendations:** Not implemented
- ⬜ **Invoice/receipt PDF generation:** Not implemented
- ⬜ **Packing slip / shipping label generation:** Not implemented
- ⬜ **Privacy policy page:** `/privacy-policy` not yet created
- ⬜ **Reorder button:** One-click reorder from customer order history

## Technical Debt
- `data/products.js` (133KB static file) kept for non-production demo pages — production pages use API
- 25+ product detail page variants exist from template — unused variants should be cleaned up
- 18 homepage themes exist — only one needed in production; rest are dead code
- `reducer/` directory is empty (no reducers needed currently)
- Placeholder contact data in Footer1.jsx (replace with real info)
- Google Maps placeholder coordinates (New York) in store location pages

## Upcoming Milestones
1. **Sprint 1 — Production Readiness:** ✅ Code complete. VPS deployment + Stripe config pending.
2. **Sprint 2 — Order Operations:** ✅ Code complete. DB migration 006 pending on VPS.
3. **Sprint 3 — Growth:** SEO meta tags, sitemap, abandoned cart, loyalty program, privacy policy page
