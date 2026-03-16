# System Patterns — Coldnb

## Architecture Style
**Modular Monolith (Frontend) + Monolithic C Backend + PostgreSQL**
- Next.js App Router (RSC + Client components mixed)
- C backend: single binary, handler-per-endpoint pattern
- No microservices; all backend logic in one process

## Dual Authentication System

```
Customer requests:                Admin requests:
  Supabase JWT                      Custom JWT
  Header: Authorization: Bearer     Header: Authorization: Bearer
  Validated in: auth_supabase.c     Validated in: auth_admin.c
  Populates: req->user_id           Populates: req->admin_id + permissions
  Used by: cart, orders,            Used by: all /api/admin/* endpoints
           wishlist, addresses
```

- **NEVER share** Supabase JWT validation logic with admin auth
- `super_admin` role bypasses all granular permission checks
- Admin permissions: per-endpoint via `hasPermission()` in `handler_admin.c`

## Backend Handler Pattern

```c
// Registration in main.c
ROUTE_GET(router, "/api/products", handler_get_products, pool);
ROUTE_POST(router, "/api/admin/login", handler_admin_login, pool);

// Handler signature
int handler_get_products(HttpRequest *req, HttpResponse *res, void *ctx);

// CRITICAL: Copy strings from JSON before cJSON_Delete(body)
// Bug: Use-after-free on request strings if not copied before JSON tree is freed
char *email = safe_strdup(cJSON_GetStringValue(cJSON_GetObjectItem(body, "email")));
cJSON_Delete(body);
// ... use email safely
free(email);
```

## Email Architecture (Immutable Split)

```
Business emails → Backend C code → Brevo HTTP API (xkeysib-...)
Auth emails     → Supabase Auth → Brevo SMTP relay (xsmtpsib-...)
```

- Do NOT mix credentials; API key ≠ SMTP password
- Add new transactional events: `svc_email.c` for abstraction, `client_brevo.c` for transport
- Keep handlers thin: pass domain data into email service, don't embed provider logic in handlers

## Frontend State Architecture

```
app/layout.js
  └── LanguageContext (i18n, localStorage "coldnb-lang")
  └── CustomerContext (cart, wishlist, compare, quickview)
        └── All customer-facing pages
  └── AdminContext (admin JWT, permissions, sidebar)
        └── app/(admin)/** pages only
  └── UserAuthContext (Supabase session)
```

- Customer context: `useContextElement()` hook
- Admin context: `useAdmin()` hook
- Language context: `useLanguage()` hook — **CLIENT ONLY** — requires `"use client"` directive

## i18n Rules (CRITICAL)

```jsx
// Pattern
"use client";
import { useLanguage } from "@/lib/i18n/LanguageContext";
const { t } = useLanguage();
t("nav.home")  // → "Início"

// String interpolation
t("cart.freeShippingSpend").replace("{amount}", "R$ 250")
```

- Always update **both** `lib/i18n/translations/pt.json` AND `en.json` simultaneously
- Same key structure required in both files
- Admin components: English only — NEVER use `useLanguage()` in admin components
- `<html lang>` updates automatically on language change (SEO)

## Admin CSS Pattern (CRITICAL)

Inside admin modals/cards, CSS classes are NOT prefixed with `admin-`:

```jsx
// CORRECT
<div className="admin-card">
  <div className="card-header">      {/* ← NOT "admin-card-header" */}
    <div className="card-body">      {/* ← NOT "admin-card-body" */}
```

Getting this wrong causes invisible/broken modal content. See `.claude/rules/ui-components.md`.

## File & Folder Naming
- **Pages:** `app/(group)/route/page.jsx` — Next.js App Router convention
- **Components:** PascalCase (`ProductCard1.jsx`, `AdminSidebar.jsx`)
- **Contexts:** `Context.jsx`, `AdminContext.jsx`, `UserAuthContext.jsx`, `ThemeContext.jsx`
- **Backend handlers:** `handler_<domain>.c` (e.g., `handler_products.c`, `handler_admin_orders.c`)
- **Backend services:** `svc_<name>.c` (e.g., `svc_email.c`, `svc_jwt.c`)
- **Backend clients:** `client_<provider>.c` (e.g., `client_brevo.c`, `client_stripe.c`)
- **SQL migrations:** `00N_description.sql` (sequential numbering, applied in order)
- **Utilities (frontend):** `utlis/` — typo is intentional (do not rename)

## API Route Structure

```
Customer-facing (Supabase auth required for protected):
  GET  /api/products            - list products (public)
  GET  /api/products/:id        - single product (public)
  GET  /api/products/search     - search products (public)
  GET  /api/products/:id/recommendations - product recommendations (public)
  GET  /api/categories          - list categories (public)
  POST /api/cart                - add to cart (auth)
  GET  /api/cart                - get cart (auth)
  POST /api/orders              - create order (auth)
  GET  /api/orders/:id          - get order (auth)
  PUT  /api/orders/:id/cancel   - cancel order (auth, pending/processing only)
  POST /api/guest-orders        - guest checkout (public, no auth)
  GET  /api/track-order         - public order tracking (order_number + email)
  POST /api/returns             - request return (auth, delivered orders only)
  GET  /api/returns             - list my returns (auth)
  GET  /api/returns/:id         - get return detail (auth)
  GET  /api/loyalty/balance     - loyalty points balance (auth)
  GET  /api/loyalty/history     - loyalty points history (auth)
  GET  /api/loyalty/rewards     - available rewards (auth)
  POST /api/loyalty/redeem      - redeem reward (auth)
  POST /api/contact             - submit contact form (public)
  POST /api/newsletter/subscribe - subscribe to newsletter (public)
  GET  /api/discount-codes/check - validate discount code (public)
  GET  /api/email/unsubscribe   - one-click email unsubscribe (public, HMAC-verified)

Admin (custom JWT required for all):
  POST /api/admin/login         - admin login
  GET  /api/admin/products      - list products
  POST /api/admin/products      - create product
  PUT  /api/admin/products/:id  - update product
  GET  /api/admin/orders        - list orders
  PUT  /api/admin/orders/:id/status - update order status + sends email + auto-awards loyalty on delivery
  PUT  /api/admin/orders/:id/tracking - add tracking number + send shipped email
  GET  /api/admin/returns       - list return requests
  PUT  /api/admin/returns/:id/status - update return status
  GET  /api/admin/customers     - list customers
  GET  /api/admin/customers/:id - customer detail
  GET  /api/admin/analytics     - analytics data
  POST /api/admin/upload        - image upload (→ public/uploads/products/)
  GET  /api/admin/discounts     - list discount codes
  POST /api/admin/discounts     - create discount code
  PUT  /api/admin/discounts/:id - update discount code
  DELETE /api/admin/discounts/:id - delete discount code
  GET  /api/admin/newsletter/subscribers - list newsletter subscribers
  DELETE /api/admin/newsletter/subscribers/:id - delete subscriber
  GET  /api/admin/contacts      - list contact submissions
  PUT  /api/admin/contacts/:id/read - mark contact as read
  GET  /api/admin/abandoned-carts - list eligible abandoned carts
  POST /api/admin/abandoned-carts/send - send recovery emails
  POST /api/admin/loyalty/grant - manually grant points
  GET  /api/admin/loyalty/rewards - list rewards catalog
  POST /api/admin/loyalty/rewards - create reward
  PUT  /api/admin/loyalty/rewards/:id - update reward
  DELETE /api/admin/loyalty/rewards/:id - delete reward
```

## Data Flow: Product Display
```
Production path:
  lib/shopApi.js → GET /api/products → C Backend → PostgreSQL
    → transformProduct() maps backend shape → frontend shape
    → ProductCard1.jsx renders (stock badge, sold-out state, price, sale %)

Legacy path (template demo pages only):
  data/products.js (static, 133KB) → some homepage themes and demo components
```
All production shop pages use the API. Static file remains only for template demo pages.

## Error Handling Conventions
- **Backend:** JSON error responses `{"error": "message"}` with appropriate HTTP status codes
- **Frontend admin:** Axios interceptor handles 401 (auto-logout), 403/404/500 (console error)
- **Frontend customer:** Toast notifications via react-hot-toast for cart/wishlist actions

## Patterns Strictly FORBIDDEN
- NEVER bind backend to `0.0.0.0` in production
- NEVER embed secrets in source code, config files committed to git, or documentation
- NEVER use `useLanguage()` in admin components or server components
- NEVER modify admin modal inner CSS to use `admin-` prefix on nested elements
- NEVER run DB migrations without explicit human confirmation (no automated migrations)
- NEVER use `memchr` for null-byte checking on strings already validated by `strlen`
