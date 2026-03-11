# Coding Standards — Coldnb

## Frontend (Next.js / React)

### File Naming
- Pages: `app/(group)/route/page.jsx`
- Components: `PascalCase.jsx` (e.g., `ProductCard1.jsx`, `AdminSidebar.jsx`)
- Contexts: `Context.jsx`, `AdminContext.jsx`, `UserAuthContext.jsx`
- Utilities: camelCase in `utlis/` (note: intentional typo — do not rename)
- Data files: camelCase in `data/` (e.g., `products.js`, `collections.js`)
- i18n translations: `lib/i18n/translations/pt.json` and `en.json`

### Component Rules
- Admin components: NEVER use `useLanguage()`, NEVER add i18n strings
- Customer-facing components: require `"use client"` if using any context hook
- `useLanguage()` is strictly client-side — never call from server components
- Always add `"use client"` as the very first line (before all imports)
- Admin API calls: always via `lib/adminApi.js` instance, not raw fetch/axios

### i18n Standards
- Always update BOTH `pt.json` AND `en.json` with identical key structure
- Namespaces: nav, cart, checkout, shopCart, product, shop, myAccount, orders, blog, homepage, contact, features, footer, newsletter, login, register, wishlist, compare, modal, admin (reserved for future), etc.
- New namespace pattern: add top-level key in both files simultaneously
- String interpolation: `t("key").replace("{var}", value)` — no template literal syntax in translation values

### CSS / Styling
- Use Bootstrap 5.3.2 utility classes first
- Custom styles in `public/scss/` component partials
- CSS custom properties for brand colors (see `--primary-color`, etc.)
- Admin modal pattern: outer wrapper uses `admin-*` class, inner elements use plain Bootstrap classes only
- Never mix admin and customer CSS scopes

### State Patterns
- Cart and wishlist: `localStorage` (guest) + server-side (authenticated) — both exist in parallel
- No Redux or Zustand — pure React Context API
- Admin token stored in `localStorage` as `adminToken` key
- Language stored in `localStorage` as `coldnb-lang` key

---

## Backend (C)

### File Naming
- Handlers: `src/handlers/handler_<domain>.c` + `include/handlers/handler_<domain>.h`
- Services: `src/services/svc_<name>.c` + `include/services/svc_<name>.h`
- External clients: `src/clients/client_<provider>.c` + `include/clients/client_<provider>.h`
- Utilities: `src/util/<name>_util.c`

### Handler Conventions
- One handler file per domain (products, orders, admin_orders, admin_products, contact, etc.)
- Handlers are thin: extract request params, call service/DB layer, respond
- Don't embed business logic or provider details directly in handlers
- Response format: always JSON (`application/json`)

### Memory Management
- Always `free()` dynamically allocated strings after use
- **CRITICAL:** Copy strings from cJSON before calling `cJSON_Delete()` — raw pointers become dangling
  ```c
  char *name = safe_strdup(cJSON_GetStringValue(cJSON_GetObjectItem(body, "name")));
  cJSON_Delete(body);
  // use name safely, then:
  free(name);
  ```
- DB connections: always `db_pool_acquire()` → use → `db_pool_release()`, even on error paths
- PGresult: always `PQclear(result)` after processing

### Security Conventions
- Argon2id for all password hashing — never bcrypt, MD5, SHA
- JWT secrets: minimum 32 bytes, generated with `openssl rand -base64 32`
- No hardcoded credentials in source files
- Validate all input: check for NULL, check length bounds before use
- Rate limiting middleware applied globally — don't bypass it

### Error Handling
- Check return values from all DB queries
- Log errors with context: `log_error("handler_create_order: DB query failed: %s", PQerrorMessage(result))`
- Return appropriate HTTP status codes: 400 (bad input), 401 (unauth), 403 (forbidden), 404 (not found), 500 (server error)
- Never expose internal error details (DB errors, stack traces) in API responses

### Build
- Format code before committing: `make format` (clang-format)
- Run static analysis: `make check` (clang-tidy)
- Debug builds for development: `make debug`; release builds for production: `make` / `make release`

---

## Database (PostgreSQL)

### Migrations
- Sequential: `001_`, `002_`, etc.
- Idempotent: use `IF NOT EXISTS` and `ON CONFLICT DO NOTHING`
- Never modify applied migrations — always create a new file
- Confirm with human before applying to production

### Schema Conventions
- UUIDs for user-facing IDs (`uuid_generate_v4()`)
- SERIAL for internal relational IDs (categories, products, etc.)
- Always include `created_at` with `DEFAULT CURRENT_TIMESTAMP`
- Include `updated_at` + trigger for mutable tables
- Snapshot pattern in `orders` and `order_items`: store denormalized data at order time (name, price, image) for historical accuracy

### Query Safety
- Always use parameterized queries ($1, $2, ...) — never string interpolation
- Indexes on: slug columns, foreign keys, frequently filtered columns
- For `psql -h 127.0.0.1` is required on VPS (peer auth fails without it)
