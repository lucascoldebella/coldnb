# Coldnb Developer Commands

Quick reference for common development tasks in the coldnb e-commerce platform.

---

## Frontend (Next.js)

Working directory: `/home/lucas/coldnb/coldnb main/coldnb nextjs`

### Start Development Server

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run dev
```

Frontend runs on `http://localhost:3000`

### Build for Production

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run build
```

Creates optimized production bundle in `.next/`

### Start Production Server

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run start
```

Runs production build (requires `npm run build` first)

### Clear Next.js Cache

When experiencing strange build or runtime issues, clear the cache:

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
rm -rf .next node_modules package-lock.json
npm install
npm run dev
```

Or just clear `.next`:

```bash
rm -rf "/home/lucas/coldnb/coldnb main/coldnb nextjs/.next"
```

### Run Linter

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run lint
```

Checks code with ESLint

### Install/Update Dependencies

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm install
```

---

## Backend (C Server)

Working directory: `/home/lucas/coldnb/coldnb-backend`

### Build Release Version (Optimized)

```bash
cd /home/lucas/coldnb/coldnb-backend
make
```

Or explicitly:

```bash
cd /home/lucas/coldnb/coldnb-backend
make release
```

Produces: `/home/lucas/coldnb/coldnb-backend/build/bin/coldnb-server`

### Build Debug Version (With Symbols)

```bash
cd /home/lucas/coldnb/coldnb-backend
make debug
```

Use for debugging with gdb. Produces same binary location as release.

### Clean Build Artifacts

```bash
cd /home/lucas/coldnb/coldnb-backend
make clean
```

Removes entire `build/` directory

### Run the Server

```bash
cd /home/lucas/coldnb/coldnb-backend
./build/bin/coldnb-server -c config/server.conf
```

Server runs on `http://localhost:8080`

**Foreground (see logs):**

```bash
./build/bin/coldnb-server -c config/server.conf
```

**Background (with nohup):**

```bash
nohup ./build/bin/coldnb-server -c config/server.conf > /tmp/coldnb.log 2>&1 &
```

### Check if Server is Running

```bash
curl http://localhost:8080/api/products
```

Should return JSON product list if running.

Or check process:

```bash
ps aux | grep coldnb-server
```

Or check port:

```bash
lsof -i :8080
```

### Stop the Server

Find the process:

```bash
ps aux | grep coldnb-server
```

Kill by PID:

```bash
kill <PID>
```

Or force kill:

```bash
kill -9 <PID>
```

If running in background with nohup:

```bash
pkill -f coldnb-server
```

### Restart the Server

```bash
pkill -f coldnb-server
sleep 1
cd /home/lucas/coldnb/coldnb-backend
./build/bin/coldnb-server -c config/server.conf
```

### Build and Run Tests

```bash
cd /home/lucas/coldnb/coldnb-backend
make test
```

Or just build tests:

```bash
make tests
```

Run individual test:

```bash
./build/bin/tests/test_name
```

### Code Quality

Format code with clang-format:

```bash
cd /home/lucas/coldnb/coldnb-backend
make format
```

Run static analysis:

```bash
cd /home/lucas/coldnb/coldnb-backend
make check
```

### Build Admin Password Generator Tool

```bash
cd /home/lucas/coldnb/coldnb-backend
make tools
./build/bin/generate_admin_password your_password_here
```

Outputs Argon2id hash for creating admin accounts

---

## Database

Database: `coldnb` on `localhost:5432`, User: `coldnb`

### Open psql Shell

```bash
psql -U coldnb -d coldnb
```

Or with host specified:

```bash
psql -h localhost -U coldnb -d coldnb
```

You'll be prompted for password (stored in `/home/lucas/coldnb/coldnb-backend/config/secrets/db_password`)

### Common psql Commands

List tables:

```
\dt
```

Describe table structure:

```
\d table_name
```

List databases:

```
\l
```

Exit psql:

```
\q
```

### View Database Tables

From psql shell:

```sql
\dt
```

From command line:

```bash
psql -U coldnb -d coldnb -c "\dt"
```

### Run Migrations

Migrations are in `/home/lucas/coldnb/coldnb-backend/sql/` and must be applied manually in order.

Initial schema:

```bash
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/001_initial_schema.sql
```

Admin permissions:

```bash
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/002_admin_permissions.sql
```

### Check Database Size

```bash
psql -U coldnb -d coldnb -c "SELECT pg_size_pretty(pg_database_size('coldnb'));"
```

### View Product Count

```bash
psql -U coldnb -d coldnb -c "SELECT COUNT(*) FROM products;"
```

### View Recent Orders

```bash
psql -U coldnb -d coldnb -c "SELECT id, order_number, user_id, total, created_at FROM orders ORDER BY created_at DESC LIMIT 10;"
```

### Create Admin User (super_admin)

First, generate password hash:

```bash
cd /home/lucas/coldnb/coldnb-backend
./build/bin/generate_admin_password your_password_here
```

Copy the hash output, then insert into database:

```bash
psql -U coldnb -d coldnb <<EOF
INSERT INTO admin_users (username, email, password_hash, role, full_name)
VALUES ('admin', 'admin@coldnb.com', 'PASTE_HASH_HERE', 'super_admin', 'System Admin');
EOF
```

### Reset Database

Warning: This deletes all data.

```bash
psql -U coldnb -d coldnb <<EOF
DROP SCHEMA public CASCADE;
CREATE SCHEMA public;
GRANT ALL ON SCHEMA public TO coldnb;
EOF
```

Then reapply migrations:

```bash
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/001_initial_schema.sql
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/002_admin_permissions.sql
```

---

## Troubleshooting

### Clear Next.js Cache When Things Break

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
rm -rf .next
npm run dev
```

Or aggressive clean:

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
rm -rf .next node_modules package-lock.json
npm install
npm run dev
```

### Check if Backend is Responding

Simple health check:

```bash
curl http://localhost:8080/api/products
```

Should return JSON products. If connection refused, backend is not running.

Check what's running on port 8080:

```bash
lsof -i :8080
```

### Verbose API Test

```bash
curl -v http://localhost:8080/api/products
```

Shows headers, status codes, and response

### View Backend Logs

If running in foreground:
- Logs print to console directly

If running with nohup:

```bash
tail -f /tmp/coldnb.log
```

If running as service:

```bash
journalctl -u coldnb-server -f
```

### Test Admin Login

```bash
curl -X POST http://localhost:8080/api/admin/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"your_password"}'
```

Should return JWT token

### Check Database Connection

From backend directory:

```bash
cd /home/lucas/coldnb/coldnb-backend
psql -U coldnb -d coldnb -c "SELECT 1"
```

If successful, shows `1`. If fails, check password in `config/secrets/db_password`

### Port Already in Use

Frontend (3000) already in use:

```bash
lsof -i :3000
kill <PID>
```

Backend (8080) already in use:

```bash
lsof -i :8080
kill <PID>
```

### Clear All Node Modules and Reinstall

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
rm -rf node_modules package-lock.json
npm install
```

### Rebuild Backend from Scratch

```bash
cd /home/lucas/coldnb/coldnb-backend
make clean
make debug
```

---

## Quick Start (Full Stack)

Start everything from scratch in separate terminals:

### Terminal 1: Backend

```bash
cd /home/lucas/coldnb/coldnb-backend
make
./build/bin/coldnb-server -c config/server.conf
```

Wait for "Server started" message.

### Terminal 2: Frontend

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run dev
```

Wait for "compiled successfully" message.

### Terminal 3: Database (optional, only if needed)

```bash
psql -U coldnb -d coldnb
```

Now you have:
- Frontend: `http://localhost:3000`
- Backend: `http://localhost:8080`
- Database: `localhost:5432/coldnb`

### Access Admin Dashboard

1. Open `http://localhost:3000/admin/login`
2. Login with admin credentials
3. Access dashboard at `http://localhost:3000/admin/dashboard`

### Test an API Call

From any terminal:

```bash
curl http://localhost:8080/api/products | jq
```

---

## Environment Setup

### First-Time Setup

1. Ensure PostgreSQL is running:

```bash
sudo systemctl status postgresql
```

2. Install backend dependencies (if not already):

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libmicrohttpd-dev libpq-dev libcurl4-openssl-dev libssl-dev libsodium-dev uuid-dev cjson-dev
```

3. Create database (if not exists):

```bash
createdb -U postgres coldnb
```

4. Build backend:

```bash
cd /home/lucas/coldnb/coldnb-backend
make
```

5. Install frontend dependencies:

```bash
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm install
```

6. Apply database migrations:

```bash
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/001_initial_schema.sql
psql -U coldnb -d coldnb -f /home/lucas/coldnb/coldnb-backend/sql/002_admin_permissions.sql
```

7. Create admin user (see Database section above)

8. Run the quick start (see Quick Start section)

### Verify Setup

```bash
# Check PostgreSQL
psql -U coldnb -d coldnb -c "SELECT 1"

# Check backend build
ls -la /home/lucas/coldnb/coldnb-backend/build/bin/coldnb-server

# Check frontend
ls -la "/home/lucas/coldnb/coldnb main/coldnb nextjs/node_modules"
```

---

## Useful Combined Commands

### Build Everything

```bash
cd /home/lucas/coldnb/coldnb-backend && make && cd "/home/lucas/coldnb/coldnb main/coldnb nextjs" && npm run build
```

### Clean Everything

```bash
cd /home/lucas/coldnb/coldnb-backend && make clean && cd "/home/lucas/coldnb/coldnb main/coldnb nextjs" && rm -rf .next node_modules
```

### Dev Everything (in one script)

Create `/home/lucas/coldnb/dev.sh`:

```bash
#!/bin/bash
set -e

echo "Starting backend..."
cd /home/lucas/coldnb/coldnb-backend
make debug
./build/bin/coldnb-server -c config/server.conf &
BACKEND_PID=$!

echo "Starting frontend..."
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm run dev &
FRONTEND_PID=$!

echo "Services started (PIDs: $BACKEND_PID, $FRONTEND_PID)"
echo "Frontend: http://localhost:3000"
echo "Backend: http://localhost:8080"
echo ""
echo "Press Ctrl+C to stop"

trap "kill $BACKEND_PID $FRONTEND_PID" EXIT
wait
```

Make executable:

```bash
chmod +x /home/lucas/coldnb/dev.sh
```

Run:

```bash
/home/lucas/coldnb/dev.sh
```

---

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| `Cannot find module` (frontend) | `rm -rf node_modules package-lock.json && npm install` |
| `Port 3000 in use` | `lsof -i :3000 \| grep LISTEN \| awk '{print $2}' \| xargs kill -9` |
| `Port 8080 in use` | `lsof -i :8080 \| grep LISTEN \| awk '{print $2}' \| xargs kill -9` |
| Backend won't start | Check database: `psql -U coldnb -d coldnb -c "SELECT 1"` |
| API 500 errors | Check backend logs: `tail -f /tmp/coldnb.log` |
| Frontend won't load | Check Next.js cache: `rm -rf .next && npm run dev` |
| Database locked | Restart PostgreSQL: `sudo systemctl restart postgresql` |
| Weird cache issues | Nuclear option: `make clean && npm install --force && npm run dev` |

---

**Last Updated:** 2026-02-05
