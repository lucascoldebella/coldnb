#!/bin/bash
set -e

echo "=== COLDNB Production Setup ==="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Step 1: Update system
echo ""
echo "=== Step 1: Updating system packages ==="
sudo apt update && sudo apt upgrade -y
print_status "System updated"

# Step 2: Install C build tools and backend dependencies
echo ""
echo "=== Step 2: Installing C build tools and libraries ==="
sudo apt install -y \
    build-essential \
    gcc \
    make \
    libmicrohttpd-dev \
    libpq-dev \
    libssl-dev \
    libsodium-dev \
    libcjson-dev \
    libcurl4-openssl-dev \
    uuid-dev \
    pkg-config \
    git \
    curl \
    wget
print_status "C build tools installed"

# Step 3: Install PostgreSQL
echo ""
echo "=== Step 3: Installing PostgreSQL ==="
sudo apt install -y postgresql postgresql-contrib
sudo systemctl enable postgresql
sudo systemctl start postgresql
print_status "PostgreSQL installed and started"

# Step 4: Install Node.js (LTS version via NodeSource)
echo ""
echo "=== Step 4: Installing Node.js LTS ==="
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs
print_status "Node.js $(node --version) installed"

# Step 5: Install Nginx
echo ""
echo "=== Step 5: Installing Nginx ==="
sudo apt install -y nginx
sudo systemctl enable nginx
sudo systemctl start nginx
print_status "Nginx installed and started"

# Step 6: Install PM2 for Node.js process management
echo ""
echo "=== Step 6: Installing PM2 ==="
sudo npm install -g pm2
print_status "PM2 installed"

# Step 7: Create PostgreSQL database and user
echo ""
echo "=== Step 7: Setting up PostgreSQL database ==="
print_warning "You will be prompted for a database password"
read -sp "Enter password for coldnb database user: " DB_PASSWORD
echo ""

sudo -u postgres psql <<EOF
CREATE USER coldnb WITH PASSWORD '${DB_PASSWORD}';
CREATE DATABASE coldnb OWNER coldnb;
GRANT ALL PRIVILEGES ON DATABASE coldnb TO coldnb;
\q
EOF
print_status "Database 'coldnb' and user created"

# Step 8: Run database migrations
echo ""
echo "=== Step 8: Running database migrations ==="
if [ -f "/home/lucas/coldnb/coldnb-backend/sql/001_initial_schema.sql" ]; then
    PGPASSWORD="${DB_PASSWORD}" psql -U coldnb -d coldnb -h localhost -f /home/lucas/coldnb/coldnb-backend/sql/001_initial_schema.sql
    print_status "Database schema created"
else
    print_warning "Schema file not found, skipping migration"
fi

# Step 9: Compile C backend
echo ""
echo "=== Step 9: Compiling C backend ==="
cd /home/lucas/coldnb/coldnb-backend
make clean 2>/dev/null || true
make
print_status "Backend compiled successfully"

# Step 10: Build frontend
echo ""
echo "=== Step 10: Building Next.js frontend ==="
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
npm install
npm run build
print_status "Frontend built successfully"

# Step 11: Create backend configuration
echo ""
echo "=== Step 11: Creating backend configuration ==="
mkdir -p /home/lucas/coldnb/coldnb-backend/config/secrets

cat > /home/lucas/coldnb/coldnb-backend/config/server.conf <<CONF
# ColdNB Server Configuration
server.port=8080
server.workers=4
server.max_connections=1000
server.request_timeout=30

database.host=localhost
database.port=5432
database.name=coldnb
database.user=coldnb
database.password_file=/home/lucas/coldnb/coldnb-backend/config/secrets/db_password
database.pool_size=10

cors.origins=http://localhost:3000,http://localhost,http://127.0.0.1
CONF

echo "${DB_PASSWORD}" > /home/lucas/coldnb/coldnb-backend/config/secrets/db_password
chmod 600 /home/lucas/coldnb/coldnb-backend/config/secrets/db_password
print_status "Backend configuration created"

# Step 12: Create systemd service for backend
echo ""
echo "=== Step 12: Creating systemd services ==="

sudo tee /etc/systemd/system/coldnb-backend.service > /dev/null <<SERVICE
[Unit]
Description=ColdNB Backend API Server
After=network.target postgresql.service

[Service]
Type=simple
User=lucas
WorkingDirectory=/home/lucas/coldnb/coldnb-backend
ExecStart=/home/lucas/coldnb/coldnb-backend/build/bin/coldnb-server -c /home/lucas/coldnb/coldnb-backend/config/server.conf
Restart=always
RestartSec=5
Environment=NODE_ENV=production

[Install]
WantedBy=multi-user.target
SERVICE

print_status "Backend systemd service created"

# Step 13: Configure Nginx
echo ""
echo "=== Step 13: Configuring Nginx ==="

sudo tee /etc/nginx/sites-available/coldnb > /dev/null <<'NGINX'
server {
    listen 80;
    server_name localhost;

    # Frontend - Next.js
    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_cache_bypass $http_upgrade;
    }

    # Backend API
    location /api {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # Static files caching
    location /_next/static {
        proxy_pass http://127.0.0.1:3000;
        proxy_cache_valid 60m;
        add_header Cache-Control "public, immutable";
    }
}
NGINX

sudo ln -sf /etc/nginx/sites-available/coldnb /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t && sudo systemctl reload nginx
print_status "Nginx configured"

# Step 14: Start services
echo ""
echo "=== Step 14: Starting services ==="
sudo systemctl daemon-reload
sudo systemctl enable coldnb-backend
sudo systemctl start coldnb-backend

# Start frontend with PM2
cd "/home/lucas/coldnb/coldnb main/coldnb nextjs"
pm2 delete coldnb-frontend 2>/dev/null || true
pm2 start npm --name "coldnb-frontend" -- start
pm2 save
pm2 startup systemd -u lucas --hp /home/lucas | tail -1 | sudo bash
print_status "All services started"

echo ""
echo "=========================================="
echo -e "${GREEN}Setup Complete!${NC}"
echo "=========================================="
echo ""
echo "Services running:"
echo "  - Frontend: http://localhost:3000 (via PM2)"
echo "  - Backend:  http://localhost:8080 (via systemd)"
echo "  - Nginx:    http://localhost (reverse proxy)"
echo ""
echo "Useful commands:"
echo "  pm2 status                    # Check frontend status"
echo "  pm2 logs coldnb-frontend      # View frontend logs"
echo "  sudo systemctl status coldnb-backend  # Check backend status"
echo "  sudo journalctl -u coldnb-backend -f  # View backend logs"
echo ""
