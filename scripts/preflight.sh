#!/bin/bash
# Coldnb Pre-Flight Check
# Run before starting work to verify the dev environment is ready.
# Usage: ./scripts/preflight.sh

PASS=0
FAIL=0
WARN=0

pass() { echo "  [PASS] $1"; ((PASS++)); }
fail() { echo "  [FAIL] $1"; ((FAIL++)); }
warn() { echo "  [WARN] $1"; ((WARN++)); }

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BACKEND_DIR="$PROJECT_ROOT/coldnb-backend"
FRONTEND_DIR="$PROJECT_ROOT/coldnb main/coldnb nextjs"

echo "========================================"
echo "  Coldnb Pre-Flight Check"
echo "========================================"
echo ""

# --- PostgreSQL ---
echo ">> PostgreSQL"
if command -v pg_isready &>/dev/null; then
    if pg_isready -h localhost -p 5432 &>/dev/null; then
        pass "PostgreSQL is running on localhost:5432"
    else
        fail "PostgreSQL is not responding on localhost:5432"
    fi
else
    if command -v psql &>/dev/null; then
        if psql -U coldnb -d coldnb -c "SELECT 1" &>/dev/null; then
            pass "PostgreSQL is reachable (via psql)"
        else
            fail "Cannot connect to PostgreSQL (database: coldnb, user: coldnb)"
        fi
    else
        fail "Neither pg_isready nor psql found in PATH"
    fi
fi
echo ""

# --- C Backend ---
echo ">> C Backend"
if [ -f "$BACKEND_DIR/build/bin/coldnb-server" ]; then
    pass "Backend binary exists: coldnb-server"
else
    fail "Backend binary not found — run: cd coldnb-backend && make"
fi

if curl -sf http://localhost:8080/api/products --max-time 2 &>/dev/null; then
    pass "Backend is responding on http://localhost:8080"
else
    fail "Backend is NOT responding on port 8080 — run: cd coldnb-backend && ./build/bin/coldnb-server -c config/server.conf"
fi
echo ""

# --- Secrets ---
echo ">> Secrets"
SECRETS_DIR="$BACKEND_DIR/config/secrets"
REQUIRED_SECRETS=("db_password" "admin_jwt_secret")
OPTIONAL_SECRETS=("jwt_secret" "supabase_anon_key" "stripe_secret_key" "brevo_api_key")

for secret in "${REQUIRED_SECRETS[@]}"; do
    if [ -f "$SECRETS_DIR/$secret" ] && [ -s "$SECRETS_DIR/$secret" ]; then
        pass "Secret exists: $secret"
    else
        fail "Missing or empty secret: $secret"
    fi
done

for secret in "${OPTIONAL_SECRETS[@]}"; do
    if [ -f "$SECRETS_DIR/$secret" ] && [ -s "$SECRETS_DIR/$secret" ]; then
        pass "Secret exists: $secret"
    else
        warn "Optional secret missing: $secret (ok for local dev)"
    fi
done
echo ""

# --- Frontend ---
echo ">> Frontend"
if [ -d "$FRONTEND_DIR/node_modules" ]; then
    pass "node_modules exists"
else
    fail "node_modules missing — run: cd \"$FRONTEND_DIR\" && npm install"
fi

if [ -f "$FRONTEND_DIR/.next/BUILD_ID" ]; then
    pass "Next.js build exists (.next/BUILD_ID)"
else
    warn "No Next.js build found — run: cd \"$FRONTEND_DIR\" && npm run build"
fi
echo ""

# --- Git ---
echo ">> Git"
if command -v git &>/dev/null; then
    pass "git is available"
else
    fail "git not found in PATH"
fi

if [ -d "$PROJECT_ROOT/.git" ]; then
    BRANCH=$(git -C "$PROJECT_ROOT" branch --show-current 2>/dev/null)
    pass "Git repo on branch: $BRANCH"
    DIRTY=$(git -C "$PROJECT_ROOT" status --porcelain 2>/dev/null | wc -l)
    if [ "$DIRTY" -gt 0 ]; then
        warn "$DIRTY uncommitted changes"
    fi
else
    warn "Not a git repository"
fi
echo ""

# --- Summary ---
echo "========================================"
echo "  Results: $PASS passed, $FAIL failed, $WARN warnings"
echo "========================================"

if [ "$FAIL" -gt 0 ]; then
    echo "  Fix the failures above before starting work."
    exit 1
else
    echo "  Environment is ready."
    exit 0
fi
