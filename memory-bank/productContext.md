# Product Context — Coldnb

## User Personas

### Customer (Shopper)
- Brazilian, mobile-first
- Browses by category (earrings, necklaces, rings, bracelets, etc.)
- Needs: fast product discovery, color/size selection, clear pricing, secure checkout
- Language default: PT-BR; can toggle to EN
- Auth: Supabase (email/password); guest checkout available via `POST /api/guest-orders`

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
5. **Post-purchase:** Order confirmation email → order history in `/my-account-orders` → status updates via email → cancel pending/processing orders
6. **Returns:** After delivery → request return in `/my-account-orders` → track return status in `/my-account-returns`
7. **Loyalty:** Earn points on delivered orders (1 pt/R$1) → view balance and history in `/my-account-loyalty` → redeem for discount codes
8. **Account:** Register/login → manage addresses, view orders, returns, loyalty, wishlist

### Admin Journey
1. Login → dashboard (analytics overview, revenue, recent orders)
2. Products: create/edit/archive products, manage images, variants, inventory; `is_new`/`is_sale` flags
3. Orders: view → update status → trigger customer email notification → auto-award loyalty points on delivery
4. Returns: review return requests, approve/reject, set refund amount
5. Customers: view profiles, order history
6. Marketing: manage discount codes, promotions
7. Newsletter: view/filter subscribers (active/inactive), delete subscribers
8. Contacts: view contact form submissions, mark as read, view full message
9. Email: view email architecture, sender configuration
10. Team: manage admin users and roles

## Implemented Product Decisions

- **18 homepage themes** exist (jewelry-01, jewelry-02, earrings, necklaces, rings, bracelets, watches, wedding, luxury, mens, kids, baby, birthstone, personalized, sets, pendants, anklets). Only one is the production homepage.
- **25+ product detail page variants** exist from template; production uses a subset.
- **Bilingual system (PT-BR/EN):** 500+ keys across 26 namespaces; all customer-facing pages translated; admin stays English.
- **Static product data** in `data/products.js` (133KB) for frontend display; database is the source of truth for real inventory via backend API.
- **Cart:** localStorage for anonymous/guest users, server-synced for authenticated users. On login, localStorage cart merges into server cart via `mergeCartOnLogin()`. All subsequent add/update/remove/clear operations write-through to server in the background.
- **Server-side cart:** `cart_items` table with `cartApi` (userApi.js). Context.jsx tracks `cartItemId` per item to target server operations correctly.

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
- Google Maps shows placeholder New York coordinates, not real store location
- `data/products.js` (133KB static) still used by template demo pages — production pages use backend API
