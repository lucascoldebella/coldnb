#!/bin/bash
#
# Coldnb Backend - Production Setup Script
#
# This script sets up the production environment on Ubuntu.
# Run as root: sudo ./setup_production.sh
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
echo_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo_error "Please run as root: sudo $0"
    exit 1
fi

echo_info "Starting Coldnb production setup..."

# 1. Install dependencies
echo_info "Installing dependencies..."
apt-get update
apt-get install -y \
    build-essential \
    libmicrohttpd-dev \
    libpq-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libsodium-dev \
    uuid-dev \
    libcjson-dev \
    postgresql \
    postgresql-contrib \
    nginx \
    certbot \
    python3-certbot-nginx

# 2. Create system user
echo_info "Creating coldnb system user..."
if ! id "coldnb" &>/dev/null; then
    useradd -r -s /bin/false coldnb
    echo_info "Created user 'coldnb'"
else
    echo_warn "User 'coldnb' already exists"
fi

# 3. Create directories
echo_info "Creating directories..."
mkdir -p /etc/coldnb/secrets
mkdir -p /var/log/coldnb
mkdir -p /var/www/coldnb/uploads/products
mkdir -p /opt/coldnb

# 4. Set ownership
echo_info "Setting ownership..."
chown coldnb:coldnb /etc/coldnb
chown coldnb:coldnb /etc/coldnb/secrets
chown coldnb:coldnb /var/log/coldnb
chown -R coldnb:coldnb /var/www/coldnb/uploads

# 5. Set permissions
echo_info "Setting permissions..."
chmod 700 /etc/coldnb/secrets

# 6. Create placeholder secret files
echo_info "Creating placeholder secret files..."
SECRETS=(
    "db_password"
    "supabase_jwt_secret"
    "supabase_anon_key"
    "admin_jwt_secret"
    "stripe_secret_key"
    "stripe_webhook_secret"
    "brevo_api_key"
)

for secret in "${SECRETS[@]}"; do
    if [ ! -f "/etc/coldnb/secrets/$secret" ]; then
        echo "REPLACE_WITH_ACTUAL_SECRET" > "/etc/coldnb/secrets/$secret"
        chown coldnb:coldnb "/etc/coldnb/secrets/$secret"
        chmod 600 "/etc/coldnb/secrets/$secret"
        echo_warn "Created placeholder: /etc/coldnb/secrets/$secret - UPDATE THIS!"
    else
        echo_info "Secret already exists: $secret"
    fi
done

# 7. Setup PostgreSQL
echo_info "Setting up PostgreSQL..."
if ! sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname='coldnb'" | grep -q 1; then
    echo_warn "Creating PostgreSQL user 'coldnb'..."
    echo_warn "You will be prompted for a password. Use the same password you put in /etc/coldnb/secrets/db_password"
    sudo -u postgres createuser -P coldnb
fi

if ! sudo -u postgres psql -tAc "SELECT 1 FROM pg_database WHERE datname='coldnb'" | grep -q 1; then
    echo_info "Creating database 'coldnb'..."
    sudo -u postgres createdb -O coldnb coldnb
fi

echo_info "Enabling uuid-ossp extension..."
sudo -u postgres psql -d coldnb -c "CREATE EXTENSION IF NOT EXISTS \"uuid-ossp\";"

# 8. Copy config file
echo_info "Installing configuration..."
if [ -f "deploy/server.conf.production" ]; then
    cp deploy/server.conf.production /etc/coldnb/server.conf
    chown coldnb:coldnb /etc/coldnb/server.conf
    chmod 600 /etc/coldnb/server.conf
    echo_info "Copied server.conf.production to /etc/coldnb/server.conf"
fi

# 9. Install systemd service
echo_info "Installing systemd service..."
if [ -f "deploy/coldnb-server.service" ]; then
    cp deploy/coldnb-server.service /etc/systemd/system/
    systemctl daemon-reload
    echo_info "Installed coldnb-server.service"
fi

# 10. Install nginx config
echo_info "Installing nginx configuration..."
if [ -f "deploy/nginx.conf" ]; then
    cp deploy/nginx.conf /etc/nginx/sites-available/coldnb-api

    if [ ! -L /etc/nginx/sites-enabled/coldnb-api ]; then
        ln -s /etc/nginx/sites-available/coldnb-api /etc/nginx/sites-enabled/
    fi

    echo_warn "Nginx config installed. Update server_name and run certbot before enabling."
fi

echo ""
echo_info "=============================================="
echo_info "Production setup complete!"
echo_info "=============================================="
echo ""
echo_warn "NEXT STEPS:"
echo ""
echo "1. Update secrets in /etc/coldnb/secrets/:"
echo "   - db_password (PostgreSQL password)"
echo "   - supabase_jwt_secret (from Supabase dashboard)"
echo "   - supabase_anon_key (from Supabase dashboard)"
echo "   - admin_jwt_secret (generate random 64-char string)"
echo "   - stripe_secret_key (from Stripe dashboard)"
echo "   - stripe_webhook_secret (from Stripe dashboard)"
echo "   - brevo_api_key (from Brevo dashboard)"
echo ""
echo "2. Update /etc/coldnb/server.conf:"
echo "   - Set supabase.project_url"
echo "   - Set cors.origins to your domain"
echo ""
echo "3. Run database migrations:"
echo "   sudo -u postgres psql -d coldnb -f sql/001_initial_schema.sql"
echo ""
echo "4. Generate admin password:"
echo "   cd scripts && gcc -o generate_admin_password generate_admin_password.c -lsodium"
echo "   ./generate_admin_password 'YourSecurePassword'"
echo "   # Run the generated SQL to update admin password"
echo ""
echo "5. Build and install the server:"
echo "   make release"
echo "   sudo install -m 755 build/bin/coldnb-server /usr/local/bin/"
echo ""
echo "6. Setup SSL certificate:"
echo "   sudo certbot --nginx -d api.yourdomain.com"
echo ""
echo "7. Update nginx config with your domain:"
echo "   sudo nano /etc/nginx/sites-available/coldnb-api"
echo "   sudo nginx -t && sudo systemctl reload nginx"
echo ""
echo "8. Start the service:"
echo "   sudo systemctl enable coldnb-server"
echo "   sudo systemctl start coldnb-server"
echo "   sudo systemctl status coldnb-server"
echo ""
