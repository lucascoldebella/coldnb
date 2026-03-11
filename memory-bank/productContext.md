# Product Context — Coldnb

## User Personas

### Customer (Shopper)
- Brazilian, mobile-first
- Browses by category (earrings, necklaces, rings, bracelets, etc.)
- Needs: fast product discovery, color/size selection, clear pricing, secure checkout
- Language default: PT-BR; can toggle to EN
- Auth: Supabase (email/password); guest checkout not yet implemented

### Store Admin
- Portuguese-speaking operator
- Manages products, orders, customers, promotions, shipping, marketing
- Admin dashboard is English-only (no i18n)
- Auth: custom JWT; super_admin role bypasses all permission checks

## Key UX Journeys (Must Work Flawlessly)

### Customer Journey
1. **Discovery:** Homepage → category/theme pages → product listing (filter/sort) → product detail
2. **Product Detail:** Image gallery, color/size swatch selection, quantity, add to cart/wishlist
3. **Cart:** View cart drawer, update quantities, apply coupon → go to checkout
4. **Checkout:** Shipping address → shipping method → payment (Stripe) → order confirmation
5. **Post-purchase:** Order confirmation email → order history in `/my-account-orders` → status updates via email
6. **Account:** Register/login → manage addresses, view orders, wishlist

### Admin Journey
1. Login → dashboard (analytics overview, revenue, recent orders)
2. Products: create/edit/archive products, manage images, variants, inventory
3. Orders: view → update status → trigger customer email notification
4. Customers: view profiles, order history
5. Marketing: manage discount codes, promotions, newsletter subscribers
6. Email: view email architecture, sender configuration

## Implemented Product Decisions

- **18 homepage themes** exist (jewelry-01, jewelry-02, earrings, necklaces, rings, bracelets, watches, wedding, luxury, mens, kids, baby, birthstone, personalized, sets, pendants, anklets). Only one is the production homepage.
- **25+ product detail page variants** exist from template; production uses a subset.
- **Bilingual system (PT-BR/EN):** 500+ keys across 26 namespaces; all customer-facing pages translated; admin stays English.
- **Static product data** in `data/products.js` (133KB) for frontend display; database is the source of truth for real inventory via backend API.
- **Cart:** localStorage for anonymous users + server-side for authenticated users (both exist in parallel — reconciliation is incomplete).
- **Server-side cart:** Exists in DB schema (`cart_items` table) and frontend has account pages showing orders, but cart sync between localStorage and server is incomplete.

## UX Anti-Patterns to Avoid
- Mixing admin and customer contexts/routes
- Calling `useLanguage()` from server components (causes build errors)
- Using `admin-` prefix on CSS classes inside admin modals (breaks styling)
- Adding placeholder/demo content references (Themesflat, themeforest, Vietnamese text)

## Competitor Differentiation
- Bilingual PT-BR/EN from day one (targeting Brazilian market explicitly)
- Custom C backend for performance and cost efficiency vs Node/Django
- Jewelry-specific: 18 homepage themes, category-focused landing pages

## Known Pain Points
- Cart is not synced: localStorage cart (guest) vs server-side cart (logged-in) — no merge on login
- Checkout flow is built (UI) but Stripe payment integration completeness needs verification
- No real-time stock updates on frontend (static data file vs DB)
- Contact forms use EmailJS (unconfigured); backend `/api/contact` is the real path
- Google Maps shows placeholder New York coordinates, not real store location
