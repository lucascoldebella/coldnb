---
applies_to: ["coldnb-backend/src/handlers/**", "coldnb-backend/src/**"]
---

# Backend API Route Conventions

## Handler Registration (main.c)
```c
HttpRouter *router = http_server_get_router(server);
ROUTE_GET(router, "/api/products", handler_get_products, pool);
ROUTE_POST(router, "/api/orders", handler_create_order, pool);
ROUTE_PUT(router, "/api/admin/orders/:id/status", handler_admin_update_order_status, pool);
ROUTE_PUT(router, "/api/admin/orders/:id/tracking", handler_admin_orders_update_tracking, pool);

// Public (no auth) — registered BEFORE auth middleware
ROUTE_GET(router, "/api/track-order", handler_orders_track, pool);
```
Pattern: `ROUTE_<METHOD>(router, "path", handler_fn, ctx)`

**IMPORTANT:** Public endpoints that would match an auth-protected prefix (e.g., `/api/orders/*`)
must use an alternative path. See ADR-009 in `docs/decisions.md`.

## Handler Signature
```c
int handler_name(HttpRequest *req, HttpResponse *res, void *ctx);
// ctx is always DbConnectionPool *pool
```

## Auth Middleware Pattern
```c
// Customer-protected endpoints (Supabase JWT)
if (!auth_require_user(req, res)) return MHD_YES;
// Populates req->user_id on success

// Admin-protected endpoints (custom JWT)
if (!auth_require_admin(req, res)) return MHD_YES;
// Populates req->admin_id on success

// Permission check (admin)
if (!admin_has_permission(req, "products.write")) {
    http_respond_json(res, 403, "{\"error\":\"Forbidden\"}");
    return MHD_YES;
}
```

## CRITICAL: JSON String Handling
```c
// WRONG — use-after-free bug (string becomes invalid after cJSON_Delete)
cJSON *body = cJSON_Parse(req->body);
const char *email = cJSON_GetStringValue(cJSON_GetObjectItem(body, "email"));
cJSON_Delete(body);
send_email(email);  // CRASH — email points to freed memory

// CORRECT — copy strings before freeing
cJSON *body = cJSON_Parse(req->body);
char *email = safe_strdup(cJSON_GetStringValue(cJSON_GetObjectItem(body, "email")));
cJSON_Delete(body);
send_email(email);  // safe
free(email);
```
This bug was already fixed in: `handler_contact.c`, `handler_orders.c`, `handler_admin_orders.c`

## Response Pattern
```c
// Success
http_respond_json(res, 200, json_string);

// Error
http_respond_json(res, 400, "{\"error\":\"Bad request\"}");
http_respond_json(res, 401, "{\"error\":\"Unauthorized\"}");
http_respond_json(res, 403, "{\"error\":\"Forbidden\"}");
http_respond_json(res, 404, "{\"error\":\"Not found\"}");
http_respond_json(res, 500, "{\"error\":\"Internal server error\"}");
```

## Email Service Pattern (for new transactional emails)
```c
// In handler (thin — only passes domain data)
OrderConfirmationData data = {
    .order_number = order_number,
    .customer_email = customer_email,
    .customer_name = customer_name,
    .total = order_total
};
svc_email_send_order_confirmation(email_service, &data);

// NOT: don't embed Brevo API details in handlers
// Add new send functions to svc_email.c, keep provider logic in client_brevo.c
```

## File Organization
```
src/handlers/    — one file per domain (handler_products.c, handler_orders.c, etc.)
src/services/    — business logic (svc_email.c, svc_jwt.c, svc_admin_auth.c)
src/clients/     — external API clients (client_brevo.c, client_stripe.c)
src/db/          — DB connection pool + query execution
src/auth/        — JWT validation (auth_supabase.c, auth_middleware.c)
src/middleware/  — cross-cutting concerns (rate limiting, analytics)
src/util/        — string helpers, JSON helpers, hash utils, UUID utils
```

## DB Query Pattern
```c
DbConnection *conn = db_pool_acquire(pool);
PGresult *result = db_query(conn, "SELECT * FROM products WHERE id = $1", 1, params);
// Process result
PQclear(result);
db_pool_release(pool, conn);
```

## CORS Configuration
Managed in `config/server.conf`:
```ini
cors.origins=http://localhost:3000,http://localhost:3001,https://coldnb.com
cors.methods=GET,POST,PUT,DELETE,OPTIONS
cors.headers=Content-Type,Authorization
```

## Security Rules
- Backend MUST bind to `127.0.0.1` in production (`server.bind_address=127.0.0.1`)
- Rate limiting middleware active on all endpoints
- All admin endpoints require valid custom JWT — no exceptions
- Password hashing: Argon2id via libsodium (never bcrypt, never MD5/SHA)
- JWT secrets: generate with `openssl rand -base64 32`
