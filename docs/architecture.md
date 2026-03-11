# Architecture — Coldnb

## System Overview

```
Internet
   │
   ▼
Nginx :80/:443 (rate limit, scanner block, SSL termination)
   │                    │
   ▼                    ▼
Next.js :3000      C Backend :8080
(PM2)              (systemd, loopback only)
   │                    │
   └────────────────────┤
                        ├──▶ PostgreSQL :5432
                        ├──▶ Supabase (JWT validation)
                        ├──▶ Stripe (payment processing)
                        └──▶ Brevo API (transactional email)

Supabase ──▶ Brevo SMTP (auth emails: signup, reset, magic link)
```

## Services & Components

### Frontend (Next.js 15)
- **Location:** `coldnb main/coldnb nextjs/`
- **Port (dev):** 3000; **Port (prod):** 3000 via PM2 → Nginx proxy
- **App Router** with route groups: `(homes)`, `(products)`, `(productDetails)`, `(my-account)`, `(admin)`, `(other-pages)`, `(blogs)`
- **State:** React Context API (CustomerContext, AdminContext, UserAuthContext, LanguageContext, ThemeContext)
- **API calls:** Via Axios — `lib/adminApi.js` for admin (JWT interceptor), `@supabase/supabase-js` for auth

### Backend (C / libmicrohttpd)
- **Location:** `coldnb-backend/`
- **Port:** 8080 (dev: all interfaces; prod: 127.0.0.1 only)
- **Architecture:** Single binary, handler registration pattern, connection pool, middleware chain
- **Middleware:** Rate limiting (`middleware_rate_limit.c`), analytics tracking (`middleware_analytics.c`)
- **Concurrency:** libmicrohttpd thread-per-connection model

### Database (PostgreSQL 13+)
- **Port:** 5432
- **DB/User:** `coldnb` / `coldnb`
- **Connection:** Pool managed in `db_connection.c`

### External Services
| Service | Purpose | Integration |
|---------|---------|------------|
| Supabase | Customer auth (signup, login, session management) | `@supabase/supabase-js` (frontend), JWT validation in `auth_supabase.c` (backend) |
| Stripe | Payment processing | `client_stripe.c` — **partially implemented** |
| Brevo (API) | Transactional business emails | `client_brevo.c` via HTTP API v3 |
| Brevo (SMTP) | Auth emails (routed through Supabase) | Configured in Supabase dashboard |

## Data Flow — Request Lifecycle

### Customer API Request
```
Browser → Nginx → C Backend :8080
  1. libmicrohttpd receives request
  2. Rate limit middleware check
  3. CORS headers added
  4. Router matches handler
  5. Auth middleware (if protected): validates Supabase JWT → populates req->user_id
  6. Handler executes: DB queries via pool, business logic
  7. Analytics middleware: log page/product view
  8. JSON response returned
```

### Admin API Request
```
Admin UI → Nginx → C Backend :8080
  Same flow, but auth validates custom JWT → populates req->admin_id + permissions
  admin_has_permission() checked per endpoint
```

### Checkout → Order Flow
```
Frontend: Cart → Checkout UI → POST /api/orders
  ├── Backend: Create order record (status: pending)
  ├── Backend: Create order_items (product snapshot)
  ├── Backend: Apply discount code (if any)
  ├── Backend: Send order confirmation email (customer + internal)
  └── Backend: [TODO] Initiate Stripe payment → await webhook
Stripe webhook → POST /api/stripe/webhook → Update order payment_status + status
```

### Email Flow
```
Backend business event (contact, order, status update)
  └── svc_email.c (abstraction layer)
      └── client_brevo.c (HTTP API v3, xkeysib-... key)
          └── Brevo transactional delivery

Supabase auth event (signup confirmation, password reset, magic link)
  └── Supabase Auth service
      └── Brevo SMTP relay (xsmtpsib-... password)
          └── Brevo delivery
```

## Module Dependencies (Frontend)
```
app/layout.js
  ├── LanguageContext.jsx ← lib/i18n/LanguageContext.jsx
  │     └── translations/pt.json + en.json
  ├── Context.jsx (CustomerContext)
  │     └── data/products.js (133KB static data)
  ├── AdminContext.jsx
  │     └── lib/adminApi.js → NEXT_PUBLIC_API_URL (C backend)
  └── UserAuthContext.jsx
        └── @supabase/supabase-js → Supabase project
```

## Nginx Proxy Configuration (Production)
- Port 80 → redirect to 443
- Port 443 → SSL termination (Let's Encrypt)
- `/api/*` → proxy to `127.0.0.1:8080` (C backend)
- `/*` → proxy to `127.0.0.1:3000` (Next.js)
- Rate limiting, scanner blocking, connection limits active

## Security Architecture
- UFW: only ports 22, 80, 443 open publicly
- CrowdSec: threat intelligence and automated blocking (API port 8180)
- fail2ban: SSH brute-force protection
- Backend port 8080: bound to loopback, unreachable from internet
- Secrets: stored in `config/secrets/` files, loaded at startup, never committed
- Admin passwords: Argon2id via libsodium
- JWT secrets: separate secrets for customer (Supabase-signed) vs admin (custom-signed)
