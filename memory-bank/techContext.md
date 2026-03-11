# Tech Context — Coldnb

## Full Stack

| Layer | Technology | Version | Notes |
|-------|-----------|---------|-------|
| Frontend framework | Next.js | 15.x (App Router) | `"coldnb main/coldnb nextjs"` directory |
| UI library | React | 19.0.0 | |
| Styling | Bootstrap + SCSS | 5.3.2 | Custom SCSS in `public/scss/` |
| Icons | icomoon (custom) | — | `public/fonts/icomoon.*` |
| Animations | WOW.js, Swiper | 11.1.15 (Swiper) | |
| HTTP client | Axios | 1.x | `lib/adminApi.js` (with JWT interceptor) |
| Charts | Chart.js + react-chartjs-2 | 4.x | Admin analytics |
| Date utils | date-fns | 3.x | |
| Image upload | react-dropzone + react-easy-crop | — | Admin product images |
| Auth (customers) | @supabase/supabase-js | 2.x | Supabase project: `emglehcqrqhblbgvjgav.supabase.co` |
| Backend language | C (GCC 9+) | — | `coldnb-backend/` |
| HTTP server (C) | libmicrohttpd | — | Port 8080 |
| Database driver | libpq | — | PostgreSQL client for C |
| HTTP client (C) | libcurl | — | For Brevo API, Stripe API calls |
| JSON parsing (C) | cJSON | — | |
| Crypto / hashing | libsodium + OpenSSL | — | Argon2id for admin passwords, JWT |
| UUID gen | libuuid | — | |
| Database | PostgreSQL | 13+ | DB: `coldnb`, User: `coldnb`, Port: 5432 |
| Email (transactional) | Brevo API v3 | — | `xkeysib-...` key; via `/v3/smtp/email` |
| Email (auth) | Brevo SMTP relay | — | `xsmtpsib-...` key; configured in Supabase |
| Payments | Stripe | — | `client_stripe.c` + `handler_payments.c`; card + PIX; frontend uses `@stripe/react-stripe-js` PaymentElement |
| Deployment: frontend | PM2 | — | `next start` on port 3000 |
| Deployment: backend | systemd | — | `coldnb-backend.service`; port 8080 |
| Reverse proxy | Nginx | — | Port 80/443; proxies to 3000 and 8080 |
| SSL | Let's Encrypt | — | |
| Security | UFW + CrowdSec + fail2ban | — | CrowdSec API on port 8180 |

## Environment Variables (Frontend)

```bash
# .env.local in "coldnb main/coldnb nextjs/"
NEXT_PUBLIC_SUPABASE_URL=https://emglehcqrqhblbgvjgav.supabase.co
NEXT_PUBLIC_SUPABASE_ANON_KEY=<supabase_anon_key>
NEXT_PUBLIC_API_URL=http://localhost:8080   # dev; set to backend URL in production
NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_live_...  # Stripe publishable key (required for checkout)
```

## Backend Config Keys (server.conf)

```ini
server.port=8080
server.bind_address=127.0.0.1          # NEVER 0.0.0.0 in production

database.host=localhost
database.port=5432
database.name=coldnb
database.user=coldnb
database.password_file=<path>

supabase.project_url=https://...supabase.co
supabase.jwt_secret_file=<path>

admin.jwt_secret_file=<path>
admin.session_duration=86400

stripe.secret_key_file=<path>          # required for payments
stripe.webhook_secret_file=<path>      # required for webhook verification
stripe.pix_enabled=true                # enables PIX payments (Brazil)

brevo.api_key_file=<path>              # xkeysib-... key
brevo.list_id=1
brevo.sandbox_mode=false               # set true for safe testing

email.store_name=Coldnb
email.site_url=https://coldnb.com
email.sender_email=noreply@coldnb.com
email.sender_name=Coldnb
email.reply_to_email=support@coldnb.com
email.reply_to_name=Coldnb Support
email.notification_email=email@coldnb.com
email.notification_name=Coldnb E-mail
```

## CLI Commands — Non-Obvious

```bash
# ── FRONTEND ──────────────────────────────────────────────
# Working dir NOTE: directory name contains a space — ALWAYS quote it
cd "/home/lucas/Documents/Development/coldnb/coldnb main/coldnb nextjs"

npm run dev          # Dev server → http://localhost:3000
npm run build        # Production build
npm run start        # Start prod server (requires build first)
npm run lint         # ESLint

# Clear Next.js cache (when strange build issues occur)
rm -rf .next && npm run dev

# Nuclear clean
rm -rf .next node_modules package-lock.json && npm install && npm run dev

# ── BACKEND ───────────────────────────────────────────────
cd /home/lucas/Documents/Development/coldnb/coldnb-backend

make                 # Release build (default)
make debug           # Debug build with symbols (use with gdb)
make clean           # Remove build/ directory
make test            # Build and run tests
make format          # clang-format all source files
make check           # Static analysis (clang-tidy)
make tools           # Build admin password generator

./build/bin/coldnb-server -c config/server.conf   # Run server (foreground)

# Generate Argon2id hash for admin password
./build/bin/generate_admin_password 'YourPassword'

# ── DATABASE ──────────────────────────────────────────────
# CRITICAL: -h 127.0.0.1 required; peer auth fails without it on VPS
psql -h 127.0.0.1 -U coldnb -d coldnb

# Apply migrations (ordered)
psql -U coldnb -d coldnb -f sql/001_initial_schema.sql
psql -U coldnb -d coldnb -f sql/002_admin_permissions.sql
psql -U coldnb -d coldnb -f sql/003_homepage_content.sql
psql -U coldnb -d coldnb -f sql/004_shipping_zones.sql
psql -U coldnb -d coldnb -f sql/005_navigation_menus.sql

# Reset schema (DESTRUCTIVE — deletes all data)
psql -U coldnb -d coldnb -c "DROP SCHEMA public CASCADE; CREATE SCHEMA public; GRANT ALL ON SCHEMA public TO coldnb;"

# ── VPS DEPLOY ────────────────────────────────────────────
cd ~/Documents/Development/coldnb && ./scripts/deploy.sh

# VPS SSH
ssh -i ~/.ssh/vps_prod_ed25519 root@134.209.44.188
# Or via helper: python3 ~/Documents/Development/infra/vps_ssh.py "<command>"

# VPS service management
journalctl -u coldnb-backend -f   # backend logs
pm2 logs coldnb-frontend           # frontend logs
pm2 restart coldnb-frontend
systemctl restart coldnb-backend
```

## System Dependencies (Backend Build)

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libmicrohttpd-dev libpq-dev \
  libcurl4-openssl-dev libssl-dev libsodium-dev uuid-dev cjson-dev
```

## Known Gotchas
- `psql -h 127.0.0.1` required on VPS — peer auth fails without explicit host
- `coldnb main/coldnb nextjs` — space in directory name, always quote in shell
- `utlis/` — folder name typo (not `utils/`), do not rename (it breaks imports)
- Stripe requires VPS configuration: secret key, webhook secret, publishable key in `.env.local`, webhook URL in Dashboard
- `brevo.sandbox_mode=true` prevents real email delivery for safe testing
- `is_valid_path()` in `http_server.c` had a broken `memchr` null-byte check — already fixed
