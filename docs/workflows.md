# Development Workflows — Coldnb

## Local Dev Setup (First Time)

### 1. Install System Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install build-essential libmicrohttpd-dev libpq-dev \
  libcurl4-openssl-dev libssl-dev libsodium-dev uuid-dev cjson-dev postgresql
```

### 2. Database Setup
```bash
sudo systemctl start postgresql
createdb -U postgres coldnb
createuser -U postgres coldnb
psql -U postgres -d coldnb -c "ALTER USER coldnb WITH PASSWORD 'yourpassword';"

# Apply migrations in order
psql -U coldnb -d coldnb -f coldnb-backend/sql/001_initial_schema.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/002_admin_permissions.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/003_homepage_content.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/004_shipping_zones.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/005_navigation_menus.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/006_order_tracking.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/007_guest_checkout_and_returns.sql
psql -U coldnb -d coldnb -f coldnb-backend/sql/008_abandoned_cart_and_loyalty.sql
```

### 3. Backend Setup
```bash
cd coldnb-backend

# Create secret files
mkdir -p config/secrets
echo 'yourdbpassword' > config/secrets/db_password
openssl rand -base64 32 > config/secrets/jwt_secret
openssl rand -base64 32 > config/secrets/admin_jwt_secret
# (stripe_secret_key and brevo_api_key: get from providers)

# Build
make debug

# Create admin user
./build/bin/generate_admin_password 'AdminPassword123'
# Copy hash, then:
psql -U coldnb -d coldnb -c "INSERT INTO admin_users (username, email, password_hash, full_name, role) VALUES ('admin', 'admin@coldnb.com', '\$HASH', 'Admin', 'super_admin');"

# Run
./build/bin/coldnb-server -c config/server.conf
```

### 4. Frontend Setup
```bash
cd "coldnb main/coldnb nextjs"

# Create .env.local
cat > .env.local << 'EOF'
NEXT_PUBLIC_SUPABASE_URL=https://emglehcqrqhblbgvjgav.supabase.co
NEXT_PUBLIC_SUPABASE_ANON_KEY=<your_anon_key>
NEXT_PUBLIC_API_URL=http://localhost:8080
EOF

npm install
npm run dev
```

### 5. Verify
```bash
# Backend health check
curl http://localhost:8080/api/products

# Admin login test
curl -X POST http://localhost:8080/api/admin/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"AdminPassword123"}'

# Frontend: open http://localhost:3000
# Admin: open http://localhost:3000/admin/login
```

---

## Daily Development

### Start Everything
**Terminal 1 (Backend):**
```bash
cd /home/lucas/Documents/Development/coldnb/coldnb-backend
make debug && ./build/bin/coldnb-server -c config/server.conf
```
**Terminal 2 (Frontend):**
```bash
cd "/home/lucas/Documents/Development/coldnb/coldnb main/coldnb nextjs"
npm run dev
```

### When to Rebuild Backend
- After any `.c` or `.h` file change: `make debug` (incremental, fast)
- After schema changes: `make clean && make debug` (safe)
- Before deploying: `make` (release build)

### Frontend Hot Reload
- Next.js dev server hot-reloads automatically
- If strange issues: `rm -rf .next && npm run dev`
- Nuclear clean: `rm -rf .next node_modules package-lock.json && npm install && npm run dev`

---

## Testing

### Backend Tests
```bash
cd coldnb-backend
make test           # build + run all tests
./build/bin/tests/test_auth     # run individual test
./build/bin/tests/test_products
```

### Frontend Linting
```bash
cd "coldnb main/coldnb nextjs"
npm run lint
```

### Manual API Testing
```bash
# List products
curl http://localhost:8080/api/products | jq

# Test with auth
TOKEN="eyJ..."
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/cart

# Admin endpoint
ADMIN_TOKEN=$(curl -s -X POST http://localhost:8080/api/admin/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"pass"}' | jq -r '.token')
curl -H "Authorization: Bearer $ADMIN_TOKEN" http://localhost:8080/api/admin/orders
```

---

## Deployment (VPS)

### Deploy Coldnb
```bash
cd ~/Documents/Development/coldnb
./scripts/deploy.sh
```

### Manual VPS Operations
```bash
# SSH access
ssh -i ~/.ssh/vps_prod_ed25519 root@134.209.44.188
# Or via helper script
python3 ~/Documents/Development/infra/vps_ssh.py "<command>"

# VPS service management
systemctl status coldnb-backend
systemctl restart coldnb-backend
journalctl -u coldnb-backend -f   # live backend logs

pm2 status
pm2 restart coldnb-frontend
pm2 logs coldnb-frontend

# VPS paths
# Frontend: /opt/coldnb/coldnb main/coldnb nextjs/
# Backend: /opt/coldnb/coldnb-backend/
# Backend config: /opt/coldnb/coldnb-backend/config/server.conf
# Secrets: /opt/coldnb/coldnb-backend/config/secrets/
```

### Stripe VPS Configuration
```bash
# 1. Add Stripe secret key
echo 'sk_live_...' > /opt/coldnb/coldnb-backend/config/secrets/stripe_secret_key
chmod 600 /opt/coldnb/coldnb-backend/config/secrets/stripe_secret_key

# 2. Add Stripe webhook secret
echo 'whsec_...' > /opt/coldnb/coldnb-backend/config/secrets/stripe_webhook_secret
chmod 600 /opt/coldnb/coldnb-backend/config/secrets/stripe_webhook_secret

# 3. Enable Stripe in server.conf (uncomment these lines):
#   stripe.secret_key_file=/opt/coldnb/coldnb-backend/config/secrets/stripe_secret_key
#   stripe.webhook_secret_file=/opt/coldnb/coldnb-backend/config/secrets/stripe_webhook_secret
#   stripe.pix_enabled=true

# 4. Add publishable key to frontend env
echo 'NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_live_...' >> "/opt/coldnb/coldnb main/coldnb nextjs/.env.local"

# 5. Restart services
systemctl restart coldnb-backend
cd "/opt/coldnb/coldnb main/coldnb nextjs" && npx next build && pm2 restart coldnb-frontend

# 6. In Stripe Dashboard:
#   - Enable PIX: Settings → Payment methods → PIX
#   - Add webhook: https://coldnb.com/api/webhooks/stripe
#   - Events: payment_intent.succeeded, payment_intent.payment_failed
```

### Safe Email Testing (No Real Delivery)
```bash
# On VPS, set sandbox mode in server.conf
brevo.sandbox_mode=true
# Restart backend
systemctl restart coldnb-backend
```

### Applying DB Migrations to VPS
```bash
# ALWAYS confirm with human before running on production
COLDNB_DB_PASS=$(cat /opt/coldnb/coldnb-backend/config/secrets/db_password)
PGPASSWORD=$COLDNB_DB_PASS psql -h 127.0.0.1 -U coldnb -d coldnb -f /opt/coldnb/coldnb-backend/sql/00N_new_migration.sql
```

### DB Sync (Local ↔ VPS)
```bash
# Pull production DB locally
cd ~/Documents/Development/coldnb && ./scripts/db-sync.sh pull
# Push local DB to production
cd ~/Documents/Development/coldnb && ./scripts/db-sync.sh push
```

---

## Troubleshooting

| Problem | Solution |
|---------|---------|
| Frontend `Cannot find module` | `rm -rf node_modules package-lock.json && npm install` |
| Port 3000 in use | `lsof -i :3000 \| awk 'NR>1{print $2}' \| xargs kill -9` |
| Port 8080 in use | `lsof -i :8080 \| awk 'NR>1{print $2}' \| xargs kill -9` |
| Backend won't start | `psql -U coldnb -d coldnb -c "SELECT 1"` — check DB connection |
| API 500 errors | `tail -f /tmp/coldnb.log` — check backend logs |
| `psql: peer auth failed` | Add `-h 127.0.0.1` to psql command |
| Strange Next.js build errors | `rm -rf .next && npm run dev` |
| `useLanguage called from server` | Add `"use client"` as first line of file |
