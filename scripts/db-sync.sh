#!/bin/bash
# Coldnb Database Sync Script
# Pulls production database from VPS and restores it locally.
#
# Usage:
#   ./scripts/db-sync.sh              # Full sync (drops local DB, restores from VPS)
#   ./scripts/db-sync.sh --dump-only  # Only download the dump, don't restore
#   ./scripts/db-sync.sh --schema     # Schema only (no data)
#
# Requires: ssh access to coldnb-vps, local PostgreSQL running

set -e

VPS="coldnb-vps"
LOCAL_DB="coldnb"
LOCAL_USER="coldnb"
DUMP_DIR="$(dirname "$0")/../dumps"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
DUMP_FILE="$DUMP_DIR/coldnb-$TIMESTAMP.sql"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[db-sync]${NC} $1"; }
ok()    { echo -e "${GREEN}[db-sync]${NC} $1"; }
warn()  { echo -e "${YELLOW}[db-sync]${NC} $1"; }
fail()  { echo -e "${RED}[db-sync]${NC} $1"; exit 1; }

# Parse flags
DUMP_ONLY=false
SCHEMA_ONLY=false
PG_DUMP_OPTS=""

for arg in "$@"; do
    case $arg in
        --dump-only)  DUMP_ONLY=true ;;
        --schema)     SCHEMA_ONLY=true; PG_DUMP_OPTS="--schema-only" ;;
        --help)
            echo "Usage: ./scripts/db-sync.sh [--dump-only] [--schema]"
            exit 0
            ;;
    esac
done

echo ""
echo "========================================"
echo "  Coldnb Database Sync (VPS → Local)"
echo "========================================"
echo ""

# Create dumps directory
mkdir -p "$DUMP_DIR"

# Step 1: Dump production database via SSH
if $SCHEMA_ONLY; then
    info "Dumping production schema (no data)..."
else
    info "Dumping production database..."
fi

ssh "$VPS" "PGPASSWORD=\$(cat /etc/coldnb/secrets/db_password) pg_dump -h 127.0.0.1 -U coldnb -d coldnb --no-owner --no-privileges $PG_DUMP_OPTS" > "$DUMP_FILE" 2>/dev/null

DUMP_SIZE=$(du -h "$DUMP_FILE" | cut -f1)
ok "Dump saved: $DUMP_FILE ($DUMP_SIZE)"

if $DUMP_ONLY; then
    ok "Dump-only mode. File at: $DUMP_FILE"
    exit 0
fi

# Step 2: Confirm before overwriting local DB
warn "This will DROP and recreate your local '$LOCAL_DB' database!"
read -p "Continue? (y/N) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    info "Aborted. Dump is still saved at: $DUMP_FILE"
    exit 0
fi

# Step 3: Get local DB password
if [ -f "$(dirname "$0")/../coldnb-backend/config/secrets/db_password" ]; then
    LOCAL_PASS=$(cat "$(dirname "$0")/../coldnb-backend/config/secrets/db_password")
else
    read -sp "Enter local PostgreSQL password for user '$LOCAL_USER': " LOCAL_PASS
    echo ""
fi

# Step 4: Drop and recreate local database
info "Recreating local database..."
PGPASSWORD="$LOCAL_PASS" psql -h 127.0.0.1 -U "$LOCAL_USER" -d postgres -c "
    SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = '$LOCAL_DB' AND pid <> pg_backend_pid();
" > /dev/null 2>&1 || true

sudo -u postgres psql -c "DROP DATABASE IF EXISTS $LOCAL_DB;" 2>/dev/null || \
    PGPASSWORD="$LOCAL_PASS" psql -h 127.0.0.1 -U "$LOCAL_USER" -d postgres -c "DROP DATABASE IF EXISTS $LOCAL_DB;"

sudo -u postgres psql -c "CREATE DATABASE $LOCAL_DB OWNER $LOCAL_USER;" 2>/dev/null || \
    PGPASSWORD="$LOCAL_PASS" psql -h 127.0.0.1 -U "$LOCAL_USER" -d postgres -c "CREATE DATABASE $LOCAL_DB OWNER $LOCAL_USER;"

ok "Local database recreated"

# Step 5: Restore dump
info "Restoring production data locally..."
PGPASSWORD="$LOCAL_PASS" psql -h 127.0.0.1 -U "$LOCAL_USER" -d "$LOCAL_DB" -f "$DUMP_FILE" > /dev/null 2>&1
ok "Database restored"

# Step 6: Verify
TABLE_COUNT=$(PGPASSWORD="$LOCAL_PASS" psql -h 127.0.0.1 -U "$LOCAL_USER" -d "$LOCAL_DB" -t -c "SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public';")
ok "Sync complete! $TABLE_COUNT tables in local database"

# Cleanup old dumps (keep last 5)
ls -t "$DUMP_DIR"/coldnb-*.sql 2>/dev/null | tail -n +6 | xargs rm -f 2>/dev/null || true

echo ""
ok "Local database is now a copy of production"
echo ""
