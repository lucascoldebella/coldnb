# Architecture Decisions — Coldnb

## ADR-001: C Backend Instead of Node.js/Python

**Decision:** Use a custom C HTTP server (libmicrohttpd) for the backend.

**Context:** Project needed a performant, cost-efficient backend for a jewelry e-commerce store on a small VPS (2vCPU/2GB DigitalOcean).

**Rationale:**
- Low memory footprint vs Node.js/Python alternatives
- Single binary deployment (no runtime version conflicts)
- Direct PostgreSQL access via libpq without ORM overhead
- VPS cost optimization: more headroom for traffic spikes

**Consequences:**
- Manual memory management (use-after-free bugs possible — see `cJSON_Delete()` pattern)
- No built-in ORM (raw SQL queries, manual result parsing)
- Longer feature development time vs framework-based backends
- Strong type safety for all data paths

---

## ADR-002: Dual Authentication System

**Decision:** Supabase JWT for customers, custom JWT for admins — completely separate.

**Context:** Customer auth requires managed identity (Supabase handles signup, email verification, password reset). Admin auth requires fine-grained permissions without relying on external service uptime.

**Rationale:**
- Supabase provides free managed auth for customers (signup, email confirmation, OAuth-ready)
- Admin auth needs to be self-contained (no Supabase dependency for critical operations)
- Granular permissions per admin role (super_admin, admin, editor, etc.)
- Different security requirements: customer tokens short-lived via Supabase; admin tokens 24h custom JWT

**Consequences:**
- Two validation code paths in backend (`auth_supabase.c` vs `auth_admin.c`)
- Admin and customer routes must never share auth middleware
- Admin password hashing: Argon2id (libsodium) — not Supabase

---

## ADR-003: Email Split — Backend API vs Supabase SMTP

**Decision:** Business/transactional emails via Brevo HTTP API from C backend; auth emails (signup, reset, magic link) routed through Supabase → Brevo SMTP.

**Context:** Needed transactional email for order lifecycle events; Supabase already handles auth email delivery.

**Rationale:**
- Auth email ownership belongs to Supabase Auth service — don't duplicate
- Business email (orders, contact notifications) belongs to application domain logic
- Brevo has two separate credential types (API key vs SMTP password) — keeping them separate prevents confusion
- Backend already had Brevo client for newsletter sync; extended rather than added new provider

**Consequences:**
- Two separate Brevo credential types must be tracked (`xkeysib-...` for backend, `xsmtpsib-...` for Supabase)
- Adding new auth email types goes through Supabase dashboard
- Adding new business email events goes through `svc_email.c` → `client_brevo.c`
- `brevo.sandbox_mode=true` config key for safe testing

---

## ADR-004: Static Product Data File for Frontend Browsing

**Decision:** Use `data/products.js` (133KB static file) for frontend product display and client-side filtering, while the backend `/api/products` is authoritative for admin and dynamic operations.

**Context:** Template-based project originally used static data. Backend was added later.

**Rationale:** Existing template had extensive frontend components built around the static data structure; migrating all at once was high-risk.

**Consequence (Technical Debt):** Frontend browsing is disconnected from actual database inventory. Price, stock, and product availability shown to customers may be stale. **Must be resolved before production launch.**

---

## ADR-005: Next.js App Router (Not Pages Router)

**Decision:** Use Next.js 15 App Router with route groups for layout organization.

**Rationale:**
- Route groups (`(homes)`, `(products)`, etc.) allow multiple layout variations without URL impact
- React Server Components where appropriate for static content
- Parallel route groups enable 18 homepage themes + 25+ product detail variants without route conflicts

**Consequences:**
- `"use client"` required on any component using React context (useLanguage, useContextElement, useAdmin)
- Server components cannot call `useLanguage()` — passes as prop or use client boundary

---

## ADR-006: Bootstrap 5 + Custom SCSS (No CSS-in-JS)

**Decision:** Bootstrap 5.3.2 + custom SCSS in `public/scss/` — no Tailwind, no styled-components.

**Rationale:** Template originated with Bootstrap; consistent with existing component library. Custom icomoon icon font integrated. Adding Tailwind would create styling conflicts.

**Consequences:**
- Admin modal CSS pattern: Bootstrap class names without `admin-` prefix on inner elements (critical — see ui-components.md)
- All theming via CSS custom properties and SCSS variables

---

## ADR-007: Bilingual i18n (PT-BR Primary, EN Secondary)

**Decision:** Custom React Context i18n system with JSON translation files; no i18n library (next-i18next, react-i18next).

**Rationale:** Lightweight solution for two languages only; localStorage persistence; easy to audit with 500+ keys; no additional dependencies.

**Consequences:**
- Both `pt.json` and `en.json` must always have identical key structure
- `useLanguage()` is client-only (React Context)
- Admin dashboard is English-only (no translation keys for admin)
- Adding a third language requires duplicating the JSON structure

---

## ADR-008: VPS Infrastructure (Single DigitalOcean Droplet)

**Decision:** Both Coldnb and Real Estate projects share one 2vCPU/2GB DigitalOcean droplet with port isolation.

**Coldnb:** Port 80 (Nginx), Port 8080 (backend loopback)
**Real Estate:** Port 8081 (Nginx), Port 8090 (backend loopback)

**Rationale:** Cost optimization; both projects are low-traffic at current stage.

**Consequences:** Resource contention possible at scale; need to monitor memory usage; migration to separate droplets if either project grows significantly.
