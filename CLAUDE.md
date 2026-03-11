# CLAUDE.md — Coldnb Router

Coldnb is a full-stack jewelry e-commerce platform: Next.js 15 + React 19 frontend, C/libmicrohttpd backend, PostgreSQL database. Supabase auth for customers, custom JWT for admins. Stripe payments, Brevo email (transactional + SMTP relay), deployed on DigitalOcean VPS. Stage: active development, partially production-ready.

---

## Critical Boundaries — NEVER Rules

- **NEVER** bind the backend to `0.0.0.0` in production. Always `127.0.0.1:8080`.
- **NEVER** commit secrets, API keys, or passwords to version control. All credentials live in `config/secrets/` files.
- **NEVER** run database migrations without explicit human confirmation.
- **NEVER** use `useLanguage()` in admin components or Next.js server components — it's client-only.
- **NEVER** prefix inner CSS classes with `admin-` inside admin modals. See `.claude/rules/ui-components.md`.
- **NEVER** use strings extracted from cJSON after calling `cJSON_Delete(body)` — copy them first.

---

## Core Commands

```bash
# FRONTEND — directory name has a space, always quote
cd "/home/lucas/Documents/Development/coldnb/coldnb main/coldnb nextjs"
npm run dev          # dev server → localhost:3000
npm run build        # production build
npm run lint         # ESLint
rm -rf .next && npm run dev   # fix strange cache/build issues

# BACKEND
cd /home/lucas/Documents/Development/coldnb/coldnb-backend
make                 # release build
make debug           # debug build (gdb-ready)
make clean           # remove build/
make test            # build + run tests
make tools           # build admin password generator
./build/bin/coldnb-server -c config/server.conf
./build/bin/generate_admin_password 'password'  # → Argon2id hash

# DATABASE (use -h 127.0.0.1 — peer auth fails without it)
psql -h 127.0.0.1 -U coldnb -d coldnb
psql -U coldnb -d coldnb -f sql/001_initial_schema.sql  # migrations in order

# VPS DEPLOY
cd ~/Documents/Development/coldnb && ./scripts/deploy.sh
ssh -i ~/.ssh/vps_prod_ed25519 root@134.209.44.188
```

---

## Memory Bank Protocol

**MEMORY BANK PROTOCOL:** At the start of EVERY new session, or when switching to a significantly different task domain, you MUST silently read ALL files in the `memory-bank/` directory before taking any action. Before ending a session or completing a major feature, you MUST update `memory-bank/activeContext.md` and `memory-bank/progress.md` to accurately reflect current state. This is non-negotiable.

---

## Context Routing Table

| Topic | Read This File |
|---|---|
| System architecture & data flow | `docs/architecture.md` |
| Coding standards & conventions | `docs/coding-standards.md` |
| Dev setup, testing, deploy | `docs/workflows.md` |
| Architecture decisions & rationale | `docs/decisions.md` |
| DB migration rules | `.claude/rules/db-migrations.md` |
| Admin UI / modal CSS patterns | `.claude/rules/ui-components.md` |
| Backend API route conventions | `.claude/rules/api-routes.md` |
| Project scope & requirements | `memory-bank/projectbrief.md` |
| UX journeys & product decisions | `memory-bank/productContext.md` |
| Stack, env vars, CLI commands | `memory-bank/techContext.md` |
| Architecture patterns & NEVER rules | `memory-bank/systemPatterns.md` |
| Current task state & open issues | `memory-bank/activeContext.md` |
| Feature status & roadmap | `memory-bank/progress.md` |
| Tool-agnostic project reference | `PROJECT_CONTEXT.md` |

---

## Working Rules

- Propose a plan and get confirmation before changes spanning 3+ files
- After completing a task, update `memory-bank/activeContext.md` and `memory-bank/progress.md`
- When adding new i18n strings: update BOTH `pt.json` AND `en.json` simultaneously
- If instructions are ambiguous, ask one clarifying question before proceeding
