#!/bin/bash
#
# Coldnb Backend - Development Setup
#
# Quick setup for local development environment.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "Coldnb Development Setup"
echo "========================"
echo ""

# Check dependencies
echo "Checking dependencies..."
MISSING=""

check_dep() {
    if ! command -v "$1" &> /dev/null; then
        MISSING="$MISSING $1"
    fi
}

check_lib() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        MISSING="$MISSING lib$1-dev"
    fi
}

check_dep gcc
check_dep make
check_dep psql
check_lib libmicrohttpd
check_lib libpq
check_lib libcurl
check_lib openssl
check_lib libsodium
check_lib uuid
check_lib libcjson

if [ -n "$MISSING" ]; then
    echo "Missing dependencies:$MISSING"
    echo ""
    echo "Install with:"
    echo "  sudo apt install build-essential libmicrohttpd-dev libpq-dev \\"
    echo "    libcurl4-openssl-dev libssl-dev libsodium-dev uuid-dev libcjson-dev \\"
    echo "    postgresql postgresql-contrib"
    exit 1
fi

echo "All dependencies found!"
echo ""

# Create local config if not exists
if [ ! -f config/server.conf ]; then
    echo "Creating config/server.conf from example..."
    cp config/server.conf.example config/server.conf
fi

# Create secrets directory
mkdir -p config/secrets

# Check PostgreSQL
echo "Checking PostgreSQL..."
if ! pg_isready &>/dev/null; then
    echo "PostgreSQL is not running. Start with:"
    echo "  sudo systemctl start postgresql"
    exit 1
fi

echo "PostgreSQL is running."
echo ""

# Check database
DB_EXISTS=$(sudo -u postgres psql -tAc "SELECT 1 FROM pg_database WHERE datname='coldnb'" 2>/dev/null || echo "0")
if [ "$DB_EXISTS" != "1" ]; then
    echo "Creating database 'coldnb'..."
    read -p "Enter password for database user 'coldnb': " -s DB_PASS
    echo ""

    sudo -u postgres psql -c "CREATE USER coldnb WITH PASSWORD '$DB_PASS';" 2>/dev/null || true
    sudo -u postgres createdb -O coldnb coldnb 2>/dev/null || true
    sudo -u postgres psql -d coldnb -c "CREATE EXTENSION IF NOT EXISTS \"uuid-ossp\";"

    echo "$DB_PASS" > config/secrets/db_password
    chmod 600 config/secrets/db_password

    echo "Database created. Loading schema..."
    PGPASSWORD="$DB_PASS" psql -h localhost -U coldnb -d coldnb -f sql/001_initial_schema.sql
else
    echo "Database 'coldnb' exists."
fi

# Build
echo ""
echo "Building project..."
make debug

echo ""
echo "========================"
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "1. Update config/secrets/ with your API keys:"
echo "   - supabase_jwt_secret (from Supabase dashboard > Settings > API)"
echo "   - supabase_anon_key (from Supabase dashboard > Settings > API)"
echo "   - stripe_secret_key (from Stripe dashboard, use test key)"
echo "   - stripe_webhook_secret (from Stripe CLI: stripe listen)"
echo "   - brevo_api_key (from Brevo dashboard)"
echo "   - admin_jwt_secret (any random string, 32+ chars)"
echo ""
echo "2. Update config/server.conf with your Supabase project URL"
echo ""
echo "3. Create an admin user:"
echo "   gcc -o scripts/generate_admin_password scripts/generate_admin_password.c -lsodium"
echo "   ./scripts/generate_admin_password 'your-password'"
echo "   # Run the generated SQL against the database"
echo ""
echo "4. Start the server:"
echo "   ./build/bin/coldnb-server -c config/server.conf"
echo ""
