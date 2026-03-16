# Active Context — Coldnb

## Current Development Focus
**Sprint 7 — Cart Sync & Storefront Polish** (started 2026-03-16)
Goal: Complete cart sync between localStorage and server, verify all admin UI features.

## Sprint 7 Completed Tasks (2026-03-16)

### Cart Sync — localStorage ↔ Server Write-Through ✅
- [x] `context/Context.jsx` — Added Supabase auth tracking (sessionRef), server write-through on add/update/remove/clear
- [x] `context/Context.jsx` — New `removeFromCart()` method exported to context (was missing entirely)
- [x] `context/Context.jsx` — New `clearCart()` method that clears both localStorage and server cart
- [x] `context/UserAuthContext.jsx` — `mergeCartOnLogin()` now includes `cartItemId` in transformed items (enables server ops)
- [x] `components/modals/CartModal.jsx` — Uses `removeFromCart` from Context (was using local function bypassing sync)
- [x] `components/otherPages/ShopCart.jsx` — Uses `removeFromCart` and `updateQuantity` from Context (was bypassing sync)
- [x] `components/otherPages/Checkout.jsx` — Uses `clearCart()` instead of `setCartProducts([])` (now clears server too)

### Admin Homepage Content UI ✅ (already existed — discovered during sprint planning)
- [x] Backend: `handler_admin_homepage.c` — Full CRUD for hero slides, banners, sections, campaigns + public `GET /api/homepage`
- [x] Frontend: `/admin/main-page` — Complete management UI with HeroSlidesManager, BannersManager, SectionsManager, CampaignsManager, CategoriesManager
- [x] Homepage (`app/page.jsx`) — Server component fetching from `GET /api/homepage` with 60s revalidation + static fallback

### Admin Navigation Menus UI ✅ (already existed — discovered during sprint planning)
- [x] Backend: `handler_admin_navigation.c` — Full CRUD for menus, groups, items + public `GET /api/navigation`
- [x] Frontend: NavigationManager integrated in `/admin/main-page` with MenuEditor, NavGroupFormModal, NavItemFormModal

## Sprint 6 Summary (completed 2026-03-13)
Stock restore on cancel, SEO meta tags + JSON-LD, admin CSV exports, GDPR data export, cancellation email.

## VPS Deployment Checklist (Sprint 1–6 Go-Live)
1. [ ] **DB migration 006:** Apply `sql/006_order_tracking.sql`
2. [ ] **DB migration 007:** Apply `sql/007_guest_checkout_and_returns.sql`
3. [ ] **DB migration 008:** Apply `sql/008_abandoned_cart_and_loyalty.sql`
4. [ ] **Stripe keys:** Add publishable key to `.env.local`, secret + webhook secret to `config/secrets/`
5. [ ] **VPS server.conf:** Uncomment Stripe lines + set `stripe.pix_enabled=true`
6. [ ] **Stripe Dashboard:** Enable PIX, add webhook endpoint
7. [ ] **Nginx:** Ensure `/api/webhooks/stripe` proxied to backend :8080
8. [ ] **Deploy:** Run `./scripts/deploy.sh`
9. [ ] **Verify:** Test all flows

## Known Open Issues
- `data/products.js` still used for demo pages (cleanup later)
- Google Maps shows New York placeholder coordinates (needs real address)
- `data/singleProductSliders.js` likely unused but low impact

## Architecture Decisions (Preserved)
- Email split: Brevo API (backend business mail) vs Brevo SMTP via Supabase (auth mail) — permanent
- Unsubscribe tokens use HMAC-SHA256 keyed on store_name + sender_email (no DB token storage)
- Invoice: print-to-PDF approach (no server-side PDF library needed)
- Loyalty redemptions create single-use discount codes in the existing `discount_codes` table
- Abandoned cart emails are admin-triggered (no automatic cron), with 3-day cooldown per user
- Loyalty auto-award: 1 point per R$1, awarded once per order on delivery (idempotent)
- Stock management: auto-decrement on order creation, auto-restore on cancellation (both customer & admin)
- GDPR export: returns JSON download (profile + addresses + orders + wishlist + loyalty) — no PDF
- CSV exports: server-side CSV generation with proper escaping, served as file download
- Shipping: Haversine distance from CEP → zone-based pricing, configurable via admin panel
- Cart sync: Context.jsx tracks Supabase session via ref, fires cartApi calls in background (optimistic UI — localStorage updates instantly, server syncs async). `cartItemId` stored per item for server ops. On login, `mergeCartOnLogin()` pushes localStorage to server then loads merged result.
