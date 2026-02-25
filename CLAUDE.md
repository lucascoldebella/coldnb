# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

# Coldnb - E-commerce Platform

Full-stack jewelry e-commerce: Next.js 15 + React 19 frontend, C/libmicrohttpd backend, PostgreSQL.
Supabase auth for users, custom JWT for admins. Stripe payments, Brevo newsletters.

## Architecture
```
┌─────────────┐     ┌─────────────┐     ┌──────────────────┐     ┌────────────┐
│   Browser   │────▶│    Nginx    │────▶│  C Backend :8080 │────▶│ PostgreSQL │
│  Next.js    │     │   (proxy)   │     │  libmicrohttpd   │     │   :5432    │
└─────────────┘     └─────────────┘     └──────────────────┘     └────────────┘
       │                                         │
       │                                         ├──▶ Supabase (user auth)
       │                                         ├──▶ Stripe (payments)
       └──▶ localhost:3000 (dev)                 └──▶ Brevo (email/newsletter)
```

### Dual Authentication System
The system uses **two separate authentication mechanisms**:

1. **Customer Authentication (Supabase)**
   - For frontend users (shoppers)
   - JWT issued by Supabase, validated in `auth_supabase.c`
   - Header: `Authorization: Bearer <supabase_jwt>`
   - Populates `req->user_id` for protected endpoints
   - Used by: cart, orders, wishlist, profile, addresses

2. **Admin Authentication (Custom JWT)**
   - For admin dashboard users
   - JWT issued by backend (`svc_admin_auth.c`), Argon2id passwords
   - Header: `Authorization: Bearer <admin_jwt>`
   - Populates `req->admin_id`, `req->admin_role`, `req->admin_permissions`
   - Used by: `/api/admin/*` endpoints
   - Roles: `super_admin` (full access) or `admin` (granular permissions)

**Never mix these two systems.** Admin endpoints ignore Supabase JWTs and vice versa.

### Admin Permission System
Admins have role-based access with granular permissions:

**Roles:**
- `super_admin`: Full access, bypasses all permission checks, can manage employees
- `admin`: Granular permissions stored in `admin_users.permissions` JSONB column

**Permission Keys** (see `sql/002_admin_permissions.sql` for full list):
- Dashboard: `view_dashboard`, `customize_dashboard`
- Financial: `view_financial`, `view_revenue`, `export_financial`
- Products: `view_products`, `create_products`, `edit_products`, `delete_products`, `manage_inventory`
- Orders: `view_orders`, `view_order_details`, `update_order_status`, `cancel_orders`
- Customers: `view_customers`, `view_customer_details`, `edit_customers`
- Marketing: `view_marketing`, `view_analytics`, `manage_discounts`
- Team: `manage_team`, `create_employees`, `edit_employees`, `assign_permissions` (super_admin only)

**Checking Permissions in Backend:**
```c
// Check if user is super_admin (bypasses checks)
if (strcmp(req->admin_role, "super_admin") == 0) {
    // Allow
}
// Check specific permission
if (admin_has_permission(req->admin_permissions, "edit_products")) {
    // Allow
}
```

**Frontend Permission Check** (AdminContext):
```javascript
const { hasPermission } = useAdminContext();
if (hasPermission('edit_products')) {
  // Show edit button
}
```

## Directory Structure
```
coldnb/
├── coldnb-backend/              # C REST API server
│   ├── src/                     # Implementation (.c files)
│   │   ├── handlers/            # API endpoint handlers (14 files)
│   │   ├── auth/, services/     # Auth & business logic
│   │   ├── clients/             # Stripe, Brevo integrations
│   │   ├── db/, http/           # Database pool, HTTP server
│   │   └── middleware/, util/   # Middleware, utilities
│   ├── include/                 # Headers (.h files)
│   ├── sql/                     # Database schema
│   ├── config/                  # server.conf + secrets/
│   └── Makefile                 # Build system
└── coldnb main/coldnb nextjs/   # Next.js 15 frontend
    ├── app/                     # App Router pages
    ├── components/              # React components
    ├── context/                 # Global state (Context.jsx)
    ├── data/                    # Static product data
    └── public/                  # Assets, SCSS
```

## Backend Blueprint

### Source Files by Function
| Directory | Files | Purpose |
|-----------|-------|---------|
| `src/handlers/` | handler_products.c | Products CRUD, search, categories |
| | handler_cart.c | Cart operations (add/update/remove) |
| | handler_orders.c | Order creation, history |
| | handler_payments.c | Stripe PaymentIntent, PIX, webhooks |
| | handler_user.c | User profile management |
| | handler_addresses.c | Shipping addresses CRUD |
| | handler_wishlist.c | Wishlist operations |
| | handler_newsletter.c | Newsletter subscribe/unsubscribe |
| | handler_contact.c | Contact form submissions |
| | handler_admin.c | Admin login/logout/session |
| | handler_admin_orders.c | Admin order management |
| | handler_admin_products.c | Admin product CRUD |
| | handler_admin_analytics.c | Dashboard analytics |
| | handler_admin_employees.c | Employee CRUD, permissions (super_admin only) |
| | handler_admin_homepage.c | Homepage content CRUD (hero, banners, sections, campaigns) |
| | handler_admin_navigation.c | Navigation menu CRUD (menus, groups, items) |
| | handler_shipping.c | Shipping zone CRUD, CEP distance calculation |
| `src/auth/` | auth_supabase.c | Supabase JWT validation |
| `src/services/` | svc_admin_auth.c | Admin JWT, Argon2id passwords |
| `src/clients/` | client_stripe.c | Stripe API integration |
| | client_brevo.c | Brevo newsletter API |
| `src/db/` | db_connection.c | PostgreSQL connection pool |
| `src/http/` | http_server.c | libmicrohttpd server |
| | http_router.c | URL routing with params |
| `src/util/` | hash_util.c | Argon2id, SHA256, HMAC |

### API Endpoints
| Method | Endpoint | Auth | Handler |
|--------|----------|------|---------|
| GET | /api/products, /api/products/:id | - | Products |
| GET | /api/categories | - | Products |
| GET/POST/PUT/DELETE | /api/cart | User | Cart |
| GET/POST | /api/orders | User | Orders |
| POST | /api/payments/create-intent | User | Payments |
| POST | /api/webhooks/stripe | - | Payments |
| GET/PUT | /api/user/profile | User | User |
| CRUD | /api/addresses | User | Addresses |
| GET/POST/DELETE | /api/wishlist | User | Wishlist |
| POST | /api/newsletter/subscribe | - | Newsletter |
| GET | /api/homepage | - | AdminHomepage (public: hero slides, categories, sections, banners, campaigns) |
| GET | /api/navigation | - | AdminNavigation (public: all menus with nested groups/items) |
| POST | /api/admin/login | - | Admin |
| CRUD | /api/admin/homepage/hero-slides | Admin | AdminHomepage |
| CRUD | /api/admin/homepage/banners | Admin | AdminHomepage |
| GET/PUT | /api/admin/homepage/sections | Admin | AdminHomepage |
| POST | /api/admin/homepage/sections/:id/products | Admin | AdminHomepage |
| CRUD | /api/admin/homepage/campaigns | Admin | AdminHomepage |
| CRUD | /api/admin/navigation/menus | Admin | AdminNavigation |
| CRUD | /api/admin/navigation/groups | Admin | AdminNavigation |
| CRUD | /api/admin/navigation/items | Admin | AdminNavigation |
| CRUD | /api/admin/products | Admin | AdminProducts |
| GET/PUT | /api/admin/orders | Admin | AdminOrders |
| GET | /api/admin/analytics/* | Admin | Analytics |
| GET/POST/PUT/DELETE | /api/admin/employees | SuperAdmin | AdminEmployees |
| GET | /api/shipping/calculate | - | Shipping |
| CRUD | /api/admin/shipping/zones | Admin | Shipping |

## Frontend Blueprint

### Key Directories
| Directory | Purpose |
|-----------|---------|
| `app/(homes)/` | 18 homepage variants (jewelry themes) |
| `app/(products)/` | Shop pages (grid, list, sidebar, pagination) |
| `app/(productDetails)/` | Product detail page (single active layout) |
| `app/(my-account)/` | User account pages |
| `app/(admin)/` | Admin dashboard pages (see Admin Dashboard below) |
| `components/headers/` | Navigation/header variants |
| `components/productCards/` | Product card components |
| `components/modals/` | Cart, QuickView, Wishlist modals |
| `components/admin/` | Admin dashboard components |

### Admin Dashboard
Full-featured admin panel at `/admin` with:

**Pages** (in `app/(admin)/`):
- `/admin/login` - Login page
- `/admin/dashboard` - Customizable dashboard with stats, charts
- `/admin/financial` - Financial analytics, revenue charts
- `/admin/marketing` - Marketing intelligence
- `/admin/main-page` - Homepage content management (navigation menus, hero slides, banners, categories, products, campaigns)
- `/admin/products` - Product list, CRUD operations
- `/admin/categories` - Category management
- `/admin/orders` - Order management with status filters
- `/admin/customers` - Customer list
- `/admin/shipping` - Shipping zone CRUD with CEP distance calculation
- `/admin/team` - Employee management (super_admin only for create/edit/delete)

**State Management** (`context/AdminContext.jsx`):
- `adminUser` - Current admin user object
- `isAuthenticated` - Auth status
- `permissions` - Granular permissions object
- `hasPermission(key)` - Permission check helper
- JWT stored in localStorage, auto-refreshed

**API Integration** (`lib/adminApi.js`):
- Axios instance with JWT interceptor
- Modules: auth, products, orders, analytics, users, employees, homepage, navigation
- Automatic token refresh, error handling

### State Management (context/Context.jsx)
- `cartProducts[]` - Cart items (localStorage persisted)
- `wishList[]` - Wishlist IDs (localStorage persisted)
- `compareItem[]` - Compare items
- `quickViewItem` - Modal product view

### Filter Reducer (reducer/filterReducer.js)
- Price range, color, size, brand filters
- Sorting, pagination state

## Bilingual Content Policy (Frontend)

The website supports **Portuguese (PT-BR) as default** and **English (EN) as secondary**.

### Architecture

**Core Files:**
- `lib/i18n/LanguageContext.jsx` — React context with `useLanguage()` hook
  - `lang` — Current language ("pt-BR" or "en")
  - `setLang(newLang)` — Change language, persists to localStorage, updates `<html lang>`
  - `t(keyPath)` — Translation function using dot-notation: `t("nav.home")`, `t("cart.subtotal")`
- `lib/i18n/translations/pt.json` — 200+ Portuguese strings across 15+ namespaces
- `lib/i18n/translations/en.json` — Matching English strings (same keys)
- `app/layout.js` — Wraps all pages with `<LanguageProvider>`

**Namespaces (organized by feature):**
| Namespace | Keys | Example |
|-----------|------|---------|
| nav | home, shop, products, blog, pages, buyNow, etc. | `t("nav.home")` → "Início" |
| cart | title, addToCart, subtotal, checkout, quantity, etc. | `t("cart.addToCart")` |
| checkout | title, billingAddress, shippingMethod, total, etc. | `t("checkout.title")` |
| footer | copyright, information, aboutUs, support, contact, etc. | `t("footer.copyright")` |
| login | title, email, password, forgotPassword, loginError, etc. | `t("login.title")` |
| features | returns14Title, shippingTitle, supportTitle, etc. | `t("features.returns14Title")` |

### Adding New Translatable Strings

1. **Add to BOTH JSON files:**
   ```json
   // lib/i18n/translations/pt.json
   { "namespace": { "key": "Valor em Português" } }

   // lib/i18n/translations/en.json
   { "namespace": { "key": "English Value" } }
   ```

2. **Use in component:**
   ```jsx
   "use client";
   import { useLanguage } from "@/lib/i18n/LanguageContext";

   export default function MyComponent() {
     const { t } = useLanguage();
     return <h1>{t("namespace.key")}</h1>;
   }
   ```

3. **For string interpolation:**
   ```jsx
   const text = t("footer.copyright").replace("{year}", new Date().getFullYear());
   ```

### Rules & Guardrails

**MUST DO:**
- Always add `"use client"` to components using `useLanguage()`
- Update both PT and EN JSON files in parallel
- Use dot-notation keys: `"featureName.stringName"`
- Store language in localStorage (automatic via `setLang()`)

**NEVER DO:**
- Re-add Themesflat, themeforest.net, Tony Nguyen, Jessie Nguyen, or Vietnam references
- Mix translations and admin components (admin stays English-only)
- Use string concatenation instead of translation keys
- Modify `LanguageContext.jsx` implementation without understanding localStorage persistence

**Admin Components (Excluded):**
- `app/(admin)/*` pages remain English-only
- No `useLanguage()` calls in admin components
- Admin logic is permission-gated and should not expose user language preferences

### Component Wiring Status

**✅ Wired (using `useLanguage()` + `t()`):**
- LanguageSelect.jsx — flag toggle (BR/US), no dropdown
- LanguageContext infrastructure (pt.json, en.json, ~250 keys)
- app/layout.js — LanguageProvider wrapping all pages
- Nav.jsx — all nav items, submenu headings, promotional text
- Topbar.jsx — translated, CurrencySelect removed
- Footer1.jsx — all strings, dynamic footer links via t()
- CartModal.jsx — ~30 translated strings
- SearchModal.jsx — all search UI strings
- MobileMenu.jsx — ~49 translation calls, CurrencySelect removed
- NewsLetterModal.jsx — title, subtitle, success/error, placeholder, button
- Features.jsx, Features2.jsx — feature titles/descriptions via translation keys

**⏳ Ready for Wiring (use pattern above):**
- Login.jsx, Register.jsx, Checkout.jsx, ShopCart.jsx — customer flow pages
- Contact1.jsx, Contact2.jsx — contact pages
- my-account/*.jsx (Information, Address, AccountSidebar) — account pages
- Secondary Topbar variants (Topbar4-11) still have CurrencySelect but are NOT used on homepage

**Template for Wiring a Component:**
```jsx
"use client";
import { useLanguage } from "@/lib/i18n/LanguageContext";
// ... other imports

export default function MyComponent() {
  const { t } = useLanguage();
  // Replace all hardcoded strings with t("namespace.key")
  return <div>{t("namespace.title")}</div>;
}
```

## Database Schema (sql/001_initial_schema.sql)

### Core Tables
| Table | Key Columns |
|-------|-------------|
| users | id, supabase_id, email, full_name, phone |
| admin_users | id, username, password_hash (Argon2id), role |
| products | id, name, slug, sku, price, stock_quantity, category_id |
| categories | id, name, slug, parent_id |
| product_images | id, product_id, url, is_primary |
| product_colors | id, product_id, name, hex_code |
| product_sizes | id, product_id, name |
| cart_items | id, user_id, product_id, quantity |
| wishlist_items | id, user_id, product_id |
| orders | id, order_number, user_id, status, payment_status, total |
| order_items | id, order_id, product_id, quantity, unit_price |
| user_addresses | id, user_id, street_address, city, postal_code |
| newsletter_subscribers | id, email, is_active |
| discount_codes | id, code, discount_type, discount_value |
| navigation_menus | id, name, slug, menu_type, show_products, banner_image_url, is_active |
| navigation_groups | id, menu_id (FK CASCADE), title, translation_key, sort_order |
| navigation_items | id, group_id (FK CASCADE), label, href, image_url, badge, sort_order |

## Configuration

### Secret Files Location
```
coldnb-backend/config/secrets/
├── db_password           # PostgreSQL password
├── jwt_secret            # Supabase JWT secret
├── supabase_anon_key     # Supabase anon key
├── admin_jwt_secret      # Admin JWT signing key
├── stripe_secret_key     # (optional) Stripe API key
├── stripe_webhook_secret # (optional) Webhook validation
└── brevo_api_key         # (optional) Brevo API key
```

### Config File (config/server.conf)
- Server: port=8080, workers=4, max_connections=1000
- Database: host=localhost, port=5432, name=coldnb, pool_size=10
- CORS: origins=localhost:3000, methods=GET,POST,PUT,DELETE
- Log level: info

### Service Dependencies
- PostgreSQL 13+ on localhost:5432
- Supabase project (user auth)
- Stripe (optional, for payments)
- Brevo (optional, for newsletters)

## Credentials & Access

### Database
- Host: localhost:5432, DB: coldnb, User: coldnb
- Password: in `config/secrets/db_password`

### Admin User Setup
```bash
# Generate password hash
cd coldnb-backend
make tools
./build/bin/generate_admin_password your_password_here

# Create super_admin (full access, bypasses permission checks)
psql -U coldnb -d coldnb <<EOF
INSERT INTO admin_users (username, email, password_hash, role, full_name)
VALUES ('admin', 'admin@coldnb.com', '<hash_from_above>', 'super_admin', 'System Admin');
EOF

# Create regular admin with granular permissions (see 002_admin_permissions.sql for examples)
```

## Development Workflow

### Backend Build & Run
```bash
cd coldnb-backend

# Build
make              # Release build (optimized)
make debug        # Debug build (symbols + assertions)
make tests        # Build test executables
make test         # Build and run all tests

# Run single test (useful for TDD)
./build/bin/tests/test_name           # Run specific test after building
make tests && ./build/bin/tests/test_name  # Build and run specific test

make clean        # Remove build artifacts

# Run server
./build/bin/coldnb-server -c config/server.conf

# Development tools
make tools        # Build admin password generator
make format       # Format code with clang-format
make check        # Static analysis with cppcheck
```

### Frontend Development
```bash
cd "coldnb main/coldnb nextjs"

npm install       # Install dependencies
npm run dev       # Dev server on :3000
npm run build     # Production build
npm run start     # Start production server
npm run lint      # Run ESLint
```

### Development Scripts
Quick shortcuts for common tasks:
```bash
./scripts/dev-restart.sh    # Stop, clean cache, rebuild, start servers (recommended)
./scripts/dev-stop.sh       # Stop both servers
./scripts/preflight.sh      # Check environment health before starting work
```

**Log Files** (saved in `logs/` directory):
- `logs/backend.txt` - Current backend session logs
- `logs/frontend.txt` - Current frontend session logs
- `logs/backend.1.txt` - Previous session (last 300 lines, for crash investigation)
- `logs/frontend.1.txt` - Previous session (last 300 lines)
- Keeps last 5 sessions automatically, oldest auto-deleted

**View Live Logs:**
```bash
tail -f /home/lucas/coldnb/logs/backend.txt
tail -f /home/lucas/coldnb/logs/frontend.txt
```

### Database Migrations
Migrations are numbered sequentially in `sql/` directory. Apply manually:
```bash
psql -U coldnb -d coldnb -f coldnb-backend/sql/001_initial_schema.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/002_admin_permissions.sql
```

### CLI Tools
```bash
# Database operations (in coldnb-backend/scripts/)
./scripts/coldnb-db shell                    # Open psql shell
./scripts/coldnb-db query "SELECT..."        # Run query
./scripts/coldnb-db import products.csv      # Bulk import

# Admin operations (requires backend running)
./scripts/coldnb-admin login                 # Login (saves JWT to ~/.coldnb_token)
./scripts/coldnb-admin products list         # List products
./scripts/coldnb-admin products create       # Create product
./scripts/coldnb-admin orders list           # List orders

# Setup scripts
./scripts/dev_setup.sh                       # Local dev environment setup
./scripts/setup_production.sh                # Production deployment
./scripts/validate_config.sh                 # Validate server.conf
```

# CLAUDE.md — C Backend Programming Guidelines

This project is written in C and prioritizes:
- Safety
- Performance
- Readability
- Predictability
- Long-term maintainability

C is powerful but unforgiving. Code must be explicit, defensive, and intentional.

---

## 1. Code Organization

### Directory Structure
- src/      — implementation files (.c)
- include/  — public headers (.h)
- tests/    — unit and integration tests
- tools/    — build or helper tools
- docs/     — protocol and architecture notes

Headers expose interfaces only. Implementation details stay in `.c` files.

---

## 2. Naming Conventions

### Files
- snake_case.c
- snake_case.h

### Functions
- lowercase with underscores
  - http_server_init
  - db_connection_close

### Types
- PascalCase for structs and typedefs
  - HttpRequest
  - DbConnection

### Variables
- snake_case
- Avoid single-letter names except for loop indices

### Constants & Macros
- UPPER_SNAKE_CASE
- Prefer `const` over macros when possible

---

## 3. Functions & Design

- Keep functions small and single-purpose
- Prefer explicit inputs and outputs
- Avoid hidden side effects
- Avoid global mutable state
- One responsibility per function

Bad:
- Huge functions
- Functions that allocate, mutate, and free unrelated resources

Good:
- Predictable lifecycle
- Clear ownership of memory

---

## 4. Memory Management (Critical)

### Rules
- Every allocation has one clear owner
- Every allocation has one clear free
- Never assume allocation succeeds
- Always check return values

### Practices
- Use calloc when zero-initialization matters
- Set freed pointers to NULL
- Avoid double-free and use-after-free
- Document ownership in function comments

Example:
// Caller owns returned pointer and must free it
char *serialize_response(const HttpResponse *res);

---

## 5. Buffer Overflow & Memory Safety

### Never
- Use gets
- Use strcpy, strcat, sprintf, scanf("%s")

### Always
- Use snprintf
- Track buffer sizes explicitly
- Pass buffer size as function arguments

Prefer length-aware APIs:
- memcpy with validated sizes
- strnlen before copy

Assume all external input is hostile.

---

## 6. Input Validation & Security

- Validate all user input
- Validate lengths, formats, and ranges
- Reject early, fail fast
- Never trust:
  - Network input
  - File input
  - Environment variables

Defensive coding is mandatory.

---

## 7. Error Handling

- Check every return value
- Return explicit error codes
- Avoid silent failures
- Log errors with enough context

Preferred pattern:
- Functions return status
- Output data via pointers

Example:
int parse_request(const char *buf, size_t len, HttpRequest *out);

---

## 8. Concurrency & Thread Safety

- Assume code may become multi-threaded
- Avoid shared mutable state
- Use explicit synchronization primitives
- Document thread ownership rules

No hidden locks.
No implicit shared state.

---

## 9. Performance Principles

- Prefer stack allocation when safe
- Avoid unnecessary heap usage
- Avoid premature optimization
- Measure before optimizing

Clarity comes first. Optimize where it matters.

---

## 10. Headers & Includes

- Headers must be idempotent (include guards or pragma once)
- Do not include unused headers
- Forward declare when possible
- Headers should not include heavy dependencies

---

## 11. Compilation & Warnings

Code must compile with:
- -Wall -Wextra -Werror
- Zero warnings tolerated

Recommended hardening flags:
- -fstack-protector
- -D_FORTIFY_SOURCE=2

---

## 12. Style & Readability

- Consistent formatting
- One statement per line
- Braces always used, even for single-line blocks
- Clear spacing and indentation

Readable code is safer code.

---

## 13. Testing

- Core logic must be testable
- Prefer deterministic tests
- Test edge cases and failure paths
- No reliance on undefined behavior

---

## 14. Philosophy

- Explicit is better than clever
- Simple is better than complex
- Defensive is better than optimistic
- If behavior is unclear, document it
- If code feels fragile, rewrite it

This codebase favors correctness, safety, and clarity over shortcuts.

---

## Important Notes & Gotchas

### Backend
- **Memory management**: Every `malloc`/`calloc` needs a corresponding `free`. Use `PQclear()` for query results.
- **Path handling**: Frontend directory has spaces: `"coldnb main/coldnb nextjs"` - always quote in shell commands
- **Route registration**: Must register routes in `src/main.c` after creating handler, easy to forget
- **CORS**: Update `cors.origins` in `server.conf` when deploying
- **Connection pool**: Always call `release_db_connection(db)` even on error paths
- **JSON responses**: Use `cJSON` library, remember to `cJSON_Delete()` after use

### Frontend
- **Context persistence**: Cart and wishlist use localStorage, may not sync across tabs
- **Admin vs User routes**: Don't mix public and admin routes in same layout
- **SCSS compilation**: Styles in `public/scss/`, compiled by Sass
- **API base URL**: Hardcoded to `localhost:8080` in development, update for production

### Database
- **Migrations**: No automatic migration system, apply `.sql` files manually in order
- **UUIDs**: Both `users.id` and `users.supabase_id` exist - use `supabase_id` for auth lookups
- **JSONB permissions**: Use `->` for object access, `->>` for text in SQL queries
- **Timestamps**: Use `TIMESTAMP WITH TIME ZONE` for all timestamps

## Adding New API Endpoints

1. **Create handler files** in `src/handlers/` and `include/handlers/`
   - Follow naming: `handler_<feature>.c` and `.h`
   - Use existing handlers as templates (e.g., `handler_products.c`)

2. **Register routes** in `src/main.c`:
   ```c
   router_register(router, "GET", "/api/your-endpoint", your_handler);
   ```

3. **Handler function signature**:
   ```c
   enum MHD_Result your_handler(void *cls, struct MHD_Connection *conn,
                                const char *url, const char *method,
                                struct HttpRequest *req);
   ```

4. **Response helpers** (in `include/http/http_response.h`):
   - `send_json_response(conn, status_code, json_string)`
   - `send_error_response(conn, status_code, "error message")`
   - Always free JSON strings after sending

5. **Database access**: Get connection from pool:
   ```c
   PGconn *db = get_db_connection();
   if (!db) return send_error_response(conn, 500, "DB unavailable");
   // ... use db ...
   release_db_connection(db);
   ```

6. **Authentication**:
   - User auth: Check `req->user_id` (set by Supabase middleware)
   - Admin auth: Check `req->admin_id` and `req->admin_role`

## Working Style

### Session Start
- **Always run `./scripts/preflight.sh` first.** Fix any failures before doing anything else.
- **To start both servers with clean cache:** `./scripts/dev-restart.sh`
  - Stops any running servers, cleans caches, rebuilds backend, starts both servers
  - Logs are saved to `logs/backend.txt` and `logs/frontend.txt` (live)
  - Previous session logs saved as `logs/backend.1.txt` and `logs/frontend.1.txt` (last 300 lines)
  - Keeps last 5 sessions of logs for crash investigation
- **To stop servers only:** `./scripts/dev-stop.sh`
- **Manual backend start (if needed):** `cd coldnb-backend && ./build/bin/coldnb-server -c config/server.conf`

### Planning vs Coding
- Do NOT spend entire sessions on planning only. After a brief assessment (max 20 lines), start implementing immediately.
- If planning is explicitly requested, still aim to begin coding within the same session.
- One working file is worth more than a perfect plan.

### Verification
- After modifying a frontend file, verify with `node --check <file>` for syntax, or `cd "coldnb main/coldnb nextjs" && npm run build` periodically.
- After modifying a backend C file, run `cd coldnb-backend && make` to confirm it compiles.
- After modifying any page that calls the API, verify the API response format matches what the frontend expects.
- Never introduce CSS/SCSS imports without confirming the target file exists.

### Credentials & Data
- Do NOT change passwords, reset admin accounts, or modify secrets unless explicitly asked.
- Do NOT recreate or overwrite existing admin users.

### Scope
- When working on multiple files, change and verify one at a time rather than batching all changes.
- If a session involves more than ~10 files, break it into focused sub-tasks with verification between each.

## Changelog
Format: `[N] YYYY-MM-DD: Description` — Cycle entries 1-30, oldest deleted first.
When making changes, add entry at bottom. If 30 entries exist, delete [1] and renumber.

[1] 2026-02-03: Created comprehensive CLAUDE.md with full project blueprint
[2] 2026-02-04: Added development workflow, CLI tools, endpoint creation guide
[3] 2026-02-04: Fixed admin dashboard stability - removed duplicate interceptors in AdminContext, added event-based auth error handling, debounced localStorage operations, memoized DataTable columns/data in products/orders/team pages, added AdminErrorBoundary for crash recovery
[4] 2026-02-06: Added preflight script (scripts/preflight.sh), post-edit validation hook (.claude/hooks/post-edit-check.sh), and Working Style directives to CLAUDE.md
[5] 2026-02-09: Added single test execution commands for TDD workflow
[6] 2026-02-09: Created dev-restart.sh and dev-stop.sh scripts with log rotation (keeps last 300 lines of 5 sessions), updated .gitignore with logs/ directory, updated CLAUDE.md with new scripts documentation
[7] 2026-02-12: Implemented full "Main Page" admin section for homepage content management. Phase 1: DB migration (sql/003_homepage_content.sql) with tables homepage_hero_slides, homepage_banners, homepage_sections, homepage_section_products, homepage_campaigns + seed data. Phase 2-3: Backend C handler (handler_admin_homepage.c/.h) with public GET /api/homepage and full admin CRUD for hero slides, banners, sections, campaigns. Phase 4: Dynamic homepage rendering in app/page.jsx with server-side fetch and 60s ISR, fallback to static data. Phase 5: Frontend API module (lib/api/adminHomepage.js), AdminSidebar nav item, AdminContext permission. Phase 6: Admin UI at /admin/main-page with 4-tab interface — HeroSlidesManager, BannersManager, SectionsManager, CampaignsManager + form modals (SlideFormModal, BannerFormModal, CampaignFormModal) + ProductPicker component. All modals use correct admin CSS design system classes and include image upload via Browse button
[8] 2026-02-18: Completed comprehensive bug fix and CEP shipping feature implementation across 5 phases. Phase 1: Fixed typos (Newsletter), free shipping logic, discount calculation, hydration mismatch. Phase 2: Created shopApi.js with API client, refactored shop to use DB API instead of static data, fixed cart data consistency, implemented search modal and category filters. Phase 3: Added shipping zones SQL migration (004_shipping_zones.sql) with CEP coordinate lookup, implemented backend handler with Haversine distance calculation for public /api/shipping/calculate endpoint, added admin CRUD routes (/api/admin/shipping/*) with middleware protection. Phase 4: Created /admin/shipping page with DataTable CRUD UI, added AdminShipping API module, integrated CEP-based shipping into ShopCart.jsx and Checkout.jsx with real-time calculation on 8-digit CEP input, displays city/state/zone/price/delivery estimate. Phase 5 (Polish): Fixed newsletter close button z-index visibility, validated sale prices (oldPrice > price only), calculated discount % dynamically on product cards, added backend validation ensuring compare_at_price >= price in both create/update handlers, fixed RTL dropdown alignment and text directionality in SCSS. All changes compile cleanly with zero warnings
[9] 2026-02-18: Static content cleanup + admin main page redesign. Phase 0: Fixed critical toArray bug — replaced with extractArray(res, key) to handle backend's named-key response wrapping ({slides:[...]}, {banners:[...]}). Phase 1: Rewrote /admin/main-page from 4-tab layout to sequential flow matching website order — new HomepageSectionCard wrapper, split BannersManager into CollectionBannersManager + CountdownBannerManager, added CategoriesManager (reads from DB, links to /admin/categories), static sections 6-9 as read-only cards, campaigns collapsed at bottom. Phase 2: Added categories query to public /api/homepage C handler, updated Collections.jsx to accept dynamic data with static fallback. Phase 3: Enhanced products_tabbed section to return tab_products grouped by tab name, updated Products3.jsx to use API-driven products with static fallback. Phase 4: Deleted 33 product detail variant dirs, 29 orphaned homes component dirs, ~20 orphaned productDetails components (kept 8 active ones + their transitive deps). All data/*.js files retained (still imported). Backend compiles zero warnings, frontend builds clean.
[10] 2026-02-23: Auth flow redesign and bug fixes. Removed page-title breadcrumb strip from /login page. Fixed vertical divider glitch (.login-wrap::before hidden via :has() selector). Redesigned auth buttons: Next/Continue green (#2ecc71, smaller, right-aligned) + Back red (#e74c3c, left-aligned) with new .auth-nav-buttons layout. Hid back button on initial welcome screen. Changed name step text "Qual é o seu nome?" → "Qual seu nome completo?" (PT/EN). Deleted /register page entirely, consolidated all auth flows to /login. Removed redundant register-choose intermediate step — "Criar Conta" now goes directly to register-1. Enhanced cepLookup.js with ViaCEP primary + BrasilAPI fallback. Renamed address step to "Endereço de entrega" (delivery address). Made AuthFlow embeddable for checkout: added embedded, onComplete, embeddedTitle props; when embedded=true renders without section wrapper, calls onComplete() instead of redirecting. Updated all 17 Header files + Login.jsx + ForgotPass.jsx: /register → /login links. Updated translations (pt.json + en.json) with deliveryAddress key. Frontend builds clean, zero warnings.
[11] 2026-02-24: Eliminated static product data from all active pages — production-ready DB-only product flow. Fixed wishlist showing wrong products: Wishlist modal + page now fetch product details from API via getProductsByIds() instead of filtering static allProducts array. Fixed Compare modal + ProductCompare page with same pattern. Fixed QuickAdd modal to accept full product object instead of ID lookup. Removed allProducts import from Context.jsx; addProductToCart() now requires full product object; wishList/compareItem initialized empty instead of [1,2,3]. Updated all 10 ProductCard variants to pass full product to addProductToCart/setQuickAddItem. CartModal "You May Also Like" now fetches featured products from API instead of hardcoded products41. ProductStikyBottom accepts product prop instead of static data. Product detail page returns 404 for missing products instead of falling back to static data. Nav.jsx mega-menu products fetched from API. Converted all homepage product components (Products, Products2, Products3, Products4, ShopGram) and all 4 jewelry home variants from static imports to API fetch. Converted all 14 shop layout variants (Products2-15) from productMain to API. Fixed Breadcumb.jsx, RelatedProducts, SearchProducts, RecentProducts. Added getProductsByIds() to shopApi.js. Updated CLAUDE.md: added handler_shipping + handler_admin_homepage to backend blueprint, added shipping API endpoints, added /admin/shipping page, fixed product detail variant count. 16 remaining static imports are orphan template components not on any active page. Frontend builds clean.
[12] 2026-02-24: Full navigation menu management system. Phase 0: Cleaned data/menu.js — removed 30 dead demo items (kept 3 jewelry), emptied swatchLinks/productFeatures, pruned productLinks to 1, removed 4 broken otherPageLinks, simplified Nav.jsx Produtos mega-menu from 4→2 columns. Phase 1: DB migration (sql/005_navigation_menus.sql) with 3 tables: navigation_menus (5 top-level menus), navigation_groups (columns within menus), navigation_items (individual links), all with CASCADE FK, indexes, triggers, seeded with cleaned menu data. Phase 2: Backend C handler (handler_admin_navigation.c/.h) — public GET /api/navigation returns nested JSON (menus→groups→items), 12 admin CRUD endpoints for menus/groups/items including reorder, registered in main.c with admin middleware. Phase 3: Frontend API module (lib/api/adminNavigation.js) with adminNavMenus/adminNavGroups/adminNavItems. Phase 4: Admin UI — NavigationManager (accordion of menu cards with active/inactive toggle), MenuEditor (per-menu settings + group/item CRUD), NavItemFormModal (with image upload for mega_grid), NavGroupFormModal; integrated into /admin/main-page as collapsible section at top. Phase 5: Rewired Nav.jsx, MobileMenu.jsx, DemoModal.jsx from static imports to useNavigationData() hook (lib/hooks/useNavigationData.js) which fetches API with static fallback. Nav.jsx uses MegaGridMenu/MegaColumnsMenu/SimpleMenu sub-components. Menus can be toggled active/inactive from admin — inactive menus hidden from public navigation. data/menu.js retained as fallback. Backend zero warnings, frontend builds clean.
