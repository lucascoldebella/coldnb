# PROJECT_CONTEXT.md — Coldnb

> Tool-agnostic reference for any AI assistant, editor, or collaborator.

## What Is This?

Coldnb is a full-stack jewelry e-commerce platform targeting the Brazilian market (PT-BR primary, EN secondary). It is a direct-to-consumer online store with a customer storefront and an admin management dashboard.

**Target Users:**
- **Shoppers:** Brazilian jewelry buyers — mobile-first, bilingual PT-BR/EN UX
- **Store Admins:** Operations team managing products, orders, customers, promotions, and email

---

## Technology Stack

| Layer | Technology | Version |
|-------|-----------|---------|
| Frontend framework | Next.js (App Router) | 15.x |
| Frontend UI | React | 19.0.0 |
| Styling | Bootstrap + SCSS | 5.3.2 |
| Customer auth | Supabase | 2.x |
| HTTP client | Axios | 1.x |
| Backend language | C (GCC 9+) | — |
| Backend HTTP | libmicrohttpd | — |
| Backend DB client | libpq | — |
| Backend JSON | cJSON | — |
| Backend crypto | libsodium + OpenSSL | — |
| Database | PostgreSQL | 13+ |
| Transactional email | Brevo API v3 | — |
| Auth email | Brevo SMTP (via Supabase) | — |
| Payments | Stripe | — |
| Production process mgr | PM2 (frontend) + systemd (backend) | — |
| Reverse proxy | Nginx | — |
| Platform | DigitalOcean Ubuntu 24.04 | 2vCPU/2GB |

---

## Directory Map

```
coldnb/                              ← Project root
├── CLAUDE.md                        ← AI routing manifest (read first)
├── PROJECT_CONTEXT.md               ← This file
├── CODEBASE_ANALYSIS.md             ← Full product & technical analysis
├── memory-bank/                     ← AI working memory (read at session start)
│   ├── projectbrief.md              ← Scope, requirements, constraints
│   ├── productContext.md            ← UX journeys, personas, decisions
│   ├── techContext.md               ← Stack, env vars, CLI commands
│   ├── systemPatterns.md            ← Architecture patterns, NEVER rules
│   ├── activeContext.md             ← Current work, open issues, next steps
│   └── progress.md                  ← Feature checklist (✅ 🔄 ⬜)
├── docs/                            ← Deep technical reference
│   ├── architecture.md              ← System diagrams, data flows
│   ├── coding-standards.md          ← Naming conventions, patterns
│   ├── workflows.md                 ← Setup, testing, deployment
│   └── decisions.md                 ← Architecture Decision Records (ADRs)
├── .claude/rules/                   ← Path-scoped AI rules
│   ├── db-migrations.md             ← SQL migration rules (applies to sql/**)
│   ├── ui-components.md             ← Frontend component rules (applies to components/**)
│   └── api-routes.md                ← Backend API handler rules (applies to handlers/**)
├── coldnb main/coldnb nextjs/       ← Frontend (NOTE: space in directory name)
│   ├── app/                         ← Next.js App Router pages
│   │   ├── (homes)/                 ← 18 jewelry homepage themes
│   │   ├── (products)/              ← 7+ shop listing layouts
│   │   ├── (productDetails)/        ← 25+ product detail variants
│   │   ├── (my-account)/            ← Customer account pages
│   │   ├── (admin)/                 ← Admin dashboard pages (orders, inventory, customers, analytics, etc.)
│   │   ├── (other-pages)/           ← Cart, checkout, contact, auth pages
│   │   ├── (blogs)/                 ← Blog pages
│   │   ├── layout.js                ← Root layout (contexts, modals, Bootstrap init)
│   │   └── api/                     ← Next.js API routes (upload endpoint)
│   ├── components/                  ← Reusable UI (headers, footers, modals, cards, admin)
│   ├── context/                     ← React Contexts (customer, admin, auth, language, theme)
│   ├── lib/                         ← Utilities (adminApi.js, i18n system)
│   ├── data/                        ← Static data (products.js 133KB, collections, menu, etc.)
│   ├── public/                      ← Static assets (scss, fonts, images, uploads)
│   └── utlis/                       ← Utility functions (note: typo intentional)
└── coldnb-backend/                  ← C backend
    ├── src/                         ← Source files (.c)
    │   ├── handlers/                ← API endpoint handlers
    │   ├── services/                ← Business logic (email, JWT, admin auth)
    │   ├── clients/                 ← External API clients (Brevo, Stripe)
    │   ├── db/                      ← PostgreSQL connection pool
    │   ├── auth/                    ← JWT validation (Supabase + custom)
    │   ├── middleware/              ← Rate limiting, analytics
    │   └── util/                    ← String, JSON, hash, UUID helpers
    ├── include/                     ← Header files (mirrors src/)
    ├── sql/                         ← Database migrations (001-006)
    ├── config/                      ← server.conf + secrets/ directory
    ├── scripts/                     ← Dev setup, production setup scripts
    ├── Makefile                     ← Build system
    └── build/                       ← Compiled output (gitignored)
```

---

## Key Commands

```bash
# Frontend dev (NOTE: quoted path required — space in directory name)
cd "/home/lucas/Documents/Development/coldnb/coldnb main/coldnb nextjs"
npm run dev          # → http://localhost:3000
npm run build
npm run lint

# Backend
cd /home/lucas/Documents/Development/coldnb/coldnb-backend
make                 # release build
make debug           # debug build
./build/bin/coldnb-server -c config/server.conf   # → http://localhost:8080

# Database (MUST use -h 127.0.0.1 on VPS for peer auth)
psql -h 127.0.0.1 -U coldnb -d coldnb

# Deploy
cd ~/Documents/Development/coldnb && ./scripts/deploy.sh
```

---

## Architecture Overview

```
Browser
  │
Nginx :80/:443 (SSL, rate limiting)
  │                 │
Next.js :3000    C Backend :8080 (loopback only)
                   │
                   ├── PostgreSQL :5432
                   ├── Supabase (customer JWT validation)
                   ├── Brevo API (order/contact emails)
                   └── Stripe (payment processing)

Supabase Auth → Brevo SMTP (auth emails: signup, reset)
```

**Key architectural facts:**
- Backend MUST run on loopback in production (127.0.0.1:8080) — never public
- Two separate auth systems: Supabase JWT (customers) vs custom JWT (admins)
- Two separate email paths: Brevo HTTP API (business) vs Brevo SMTP via Supabase (auth)
- Frontend product browsing is API-driven via `lib/shopApi.js` → `GET /api/products`
- Public order tracking at `/api/track-order` (not `/api/orders/track` — see ADR-009)
- Stripe checkout with PaymentElement (card + PIX) — VPS keys pending

---

## Detailed Documentation
- System architecture & data flows → `docs/architecture.md`
- Coding standards & naming conventions → `docs/coding-standards.md`
- Dev setup, testing, deployment → `docs/workflows.md`
- Architecture decisions & rationale → `docs/decisions.md`
- Feature status (what's built, partial, planned) → `memory-bank/progress.md`
- Current work & open issues → `memory-bank/activeContext.md`
