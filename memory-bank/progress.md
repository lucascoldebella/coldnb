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
- ✅ Account pages (dashboard, addresses, orders, order details, loyalty, returns — UI complete)
- ✅ Order creation backend (`POST /api/orders`)
- ✅ Order status lifecycle (pending → confirmed → processing → shipped → delivered → cancelled)
- ✅ Customer order cancellation (`PUT /api/orders/:id/cancel`)
- ✅ Admin order management (list, filter, update status)
- ✅ Transactional email: all templates branded HTML with unsubscribe links
- ✅ Email architecture split (Brevo API for backend, SMTP relay for Supabase auth)
- ✅ Email unsubscribe (HMAC-verified one-click from email links)
- ✅ Admin email management page (`/admin/email`)
- ✅ Contact form backend (`POST /api/contact` → DB + email notification)
- ✅ Admin contact submissions page (`/admin/contacts`) with mark-as-read
- ✅ Newsletter subscriber management (DB + Brevo sync)
- ✅ Admin newsletter page (`/admin/newsletter`) with subscriber table
- ✅ Image upload API (`POST /api/admin/upload` → `public/uploads/products/`)
- ✅ Analytics tracking (page views, product views — DB tables + backend middleware)
- ✅ Backend analytics handler (`/api/admin/analytics`)
- ✅ Admin analytics page (revenue, orders, top products charts)
- ✅ Category management (DB + backend handlers)
- ✅ Discount code management (full CRUD: admin list/create/update/delete + public validation)
- ✅ Shipping zones schema (SQL migration applied)
- ✅ Navigation menus schema (SQL migration applied)
- ✅ VPS deployment (Nginx + PM2 + systemd + UFW + CrowdSec + fail2ban)
- ✅ SSL (Let's Encrypt via Nginx)
- ✅ Privacy policy page (LGPD-compliant, bilingual)
- ✅ Product recommendations (co-purchased + same category)
- ✅ Invoice/receipt page (print-optimized)
- ✅ Loyalty/rewards program (points, rewards catalog, redemption → discount codes)
- ✅ Loyalty auto-award on order delivery (1 point per R$1)
- ✅ Abandoned cart recovery (admin-triggered emails with product preview)
- ✅ Free shipping threshold (R$75+)
- ✅ Stock status display (product detail + listing cards with sold-out badge)
- ✅ Account deletion confirmation dialog
- ✅ Customer return status page (`/my-account-returns`)
- ✅ SEO sitemap (`/sitemap.xml` with static + product pages)
- ✅ Product `is_new` / `is_sale` admin form toggles
- ✅ Stock auto-decrement on order creation + restore on cancellation
- ✅ SEO meta tags (product OG tags, JSON-LD structured data, shop page metadata)
- ✅ Admin CSV exports (orders, products, customers) — `/admin/exports`
- ✅ GDPR data export (`GET /api/user/export` + frontend button)
- ✅ Cart sync: localStorage ↔ server write-through for authenticated users (add, update, remove, clear)
- ✅ Admin homepage content UI: hero slides, banners, sections, campaigns, categories (`/admin/main-page`)
- ✅ Admin navigation menus UI: menus, groups, items with reorder (`/admin/main-page`)

## Recently Completed (Sprint 7 — 2026-03-16)
- ✅ **Cart sync write-through:** Context.jsx tracks Supabase session, syncs add/update/remove/clear to server via cartApi
- ✅ **removeFromCart:** New centralized method in Context (was missing — CartModal and ShopCart had local bypass functions)
- ✅ **clearCart:** Clears both localStorage and server cart (used in Checkout after order creation)
- ✅ **cartItemId tracking:** mergeCartOnLogin includes server cart_items.id so write-through ops can target correct rows
- ✅ **Admin homepage + navigation UIs:** Verified fully functional (backend + frontend + admin page all exist)

## Recently Completed (Sprint 6 — 2026-03-13)
- ✅ **Stock restore on cancel:** Customer cancel + admin cancel now restore stock (transactional)
- ✅ **SEO meta tags:** Product detail page gets OG + Twitter + canonical, shop/search pages get metadata
- ✅ **JSON-LD:** Product detail page outputs Product schema for Google rich snippets
- ✅ **Admin CSV exports:** `GET /api/admin/exports/{orders,products,customers}` + frontend download page
- ✅ **GDPR data export:** `GET /api/user/export` returns JSON download (profile, addresses, orders, wishlist, loyalty)
- ✅ **Export My Data button:** ClientPanel.jsx with i18n support (PT + EN)
- ✅ **Cancellation email:** Customer cancel now sends status update email via existing Brevo flow
- ✅ **Admin returns modal:** Already existed — status dropdown, refund amount, admin notes
- ✅ **Shipping rates:** Already fully implemented — Haversine distance + zone-based pricing + admin CRUD

## Recently Completed (Sprint 5 — 2026-03-12)
- ✅ **Free shipping threshold:** Cart >= R$75 gets free shipping in checkout (matches CartModal promise)
- ✅ **Stock status fix:** Dynamic SKU, brand, stock status in Details1.jsx; disabled cart button when out of stock
- ✅ **Account deletion confirmation:** Two-step confirm in ClientPanel.jsx
- ✅ **Customer order cancellation:** Backend `PUT /api/orders/:id/cancel` + frontend cancel button
- ✅ **Loyalty auto-award:** Points auto-inserted on order delivery (idempotent, 1 pt/R$1)
- ✅ **Newsletter admin page:** `/admin/newsletter` with subscriber list, status filter, delete
- ✅ **Contact admin page:** `/admin/contacts` with submissions list, read/unread filter, view modal
- ✅ **Product form flags:** `is_new` and `is_sale` toggle switches in admin product form
- ✅ **Return status page:** `/my-account-returns` with status badges, both sidebars updated
- ✅ **SEO sitemap:** Dynamic `sitemap.js` with static + product pages
- ✅ **Sold-out badge:** ProductCard1.jsx shows "Sold Out" overlay + disabled add-to-cart
- ✅ **Image optimization:** Replaced raw `<img>` with `<Image>` in OrderTrac + mobile AccountSidebar
- ✅ **Unsubscribe build fix:** Wrapped useSearchParams in Suspense boundary

## Recently Completed (Sprint 4 — 2026-03-12)
- ✅ **Guest Stripe payment flow:** `POST /api/guest-payments/create-intent` public endpoint
- ✅ **Privacy policy page:** `/privacy-policy` with LGPD sections, bilingual
- ✅ **Email unsubscribe:** `GET /api/email/unsubscribe` with HMAC token verification
- ✅ **Rich HTML email templates:** Branded wrapper, all 5 email types upgraded
- ✅ **Admin discount CRUD:** Backend + frontend `/admin/discounts` page
- ✅ **Product recommendations:** `GET /api/products/:id/recommendations`
- ✅ **Abandoned cart recovery:** Admin endpoints + branded recovery email
- ✅ **Invoice generation:** `/invoice/[id]` print-optimized page
- ✅ **Loyalty program:** Full backend + frontend + DB migration 008

## Recently Completed (Sprint 3 — 2026-03-12)
- ✅ **Product search page:** `/search-result?q=` wired to `GET /api/products/search`
- ✅ **Password reset flow:** Supabase `resetPasswordForEmail()` + `/reset-password` callback
- ✅ **Reorder button:** "Buy Again" in `OrderDetails.jsx`
- ✅ **Discount code pre-validation:** `GET /api/discount-codes/check?code=XXX` (public)
- ✅ **Guest checkout:** `POST /api/guest-orders` (no auth); DB migration 007
- ✅ **Return/refund workflow:** Customer form + admin review + DB table

## Planned / Not Started ⬜
- ⬜ **Shipping rate calculation:** Real-time carrier rate lookup (Correios integration)
- ⬜ **SMS/WhatsApp notifications:** Not started (requires Meta API keys)
- ⬜ **PWA / push notifications:** Not started
- ⬜ **Social login (OAuth):** Supabase supports it but not wired in frontend
- ⬜ **Image optimization pipeline:** No CDN; images served from `public/` directly
- ⬜ **Packing slip / shipping label generation:** Not implemented

## Technical Debt
- `data/products.js` (133KB static file) kept for non-production demo pages — production pages use API
- `data/singleProductSliders.js` likely unused (low impact)
- Google Maps placeholder coordinates (New York) in contact + store location pages
- 25+ product detail page variants exist from template — production uses only `product-detail/[id]`
- 18 homepage themes exist — all are routable demo variants, actively linked in nav

## Upcoming Milestones
1. **Sprint 1 — Production Readiness:** ✅ Code complete.
2. **Sprint 2 — Order Operations:** ✅ Code complete. DB migration 006 pending on VPS.
3. **Sprint 3 — Core Functionality:** ✅ Code complete. DB migration 007 pending on VPS.
4. **Sprint 4 — Fundamentals & Polish:** ✅ Code complete. DB migration 008 pending on VPS.
5. **Sprint 5 — Core Gaps:** ✅ Code complete. Deploy pending.
6. **Sprint 6 — Core Functionality & Inventory:** ✅ Code complete. Deploy pending.
7. **Sprint 7 — Cart Sync & Storefront Polish:** ✅ Code complete. Deploy pending.
