# Coldnb E-commerce Backend

C backend for the Coldnb e-commerce platform, providing REST APIs for products, cart, orders, payments, and admin functionality.

## Quick Start (Development)

```bash
# Run the development setup script
./scripts/dev_setup.sh

# Or manually:
make debug
./build/bin/coldnb-server -c config/server.conf
```

## Quick Start (Production)

```bash
# Run the production setup script as root
sudo ./scripts/setup_production.sh

# Then follow the printed instructions to:
# 1. Configure secrets
# 2. Setup database
# 3. Create admin user
# 4. Configure SSL
# 5. Start the service
```

## Requirements

- GCC 9+ or Clang 10+
- PostgreSQL 13+
- Required libraries:
  - libmicrohttpd (HTTP server)
  - libpq (PostgreSQL client)
  - libcurl (HTTP client)
  - OpenSSL (JWT/crypto)
  - libsodium (password hashing)
  - libuuid (UUID generation)
  - cJSON (JSON parsing)

### Ubuntu/Debian Installation

```bash
sudo apt update
sudo apt install build-essential libmicrohttpd-dev libpq-dev libcurl4-openssl-dev \
    libssl-dev libsodium-dev uuid-dev libcjson-dev postgresql
```

## Building

```bash
# Release build
make

# Debug build
make debug

# Run tests
make test

# Clean
make clean
```

## Configuration

1. Copy the example configuration:
   ```bash
   sudo mkdir -p /etc/coldnb/secrets
   sudo chmod 700 /etc/coldnb/secrets
   sudo cp config/server.conf.example /etc/coldnb/server.conf
   ```

2. Create secret files:
   ```bash
   # Database password
   echo "your_db_password" | sudo tee /etc/coldnb/secrets/db_password

   # Supabase JWT secret
   echo "your_jwt_secret" | sudo tee /etc/coldnb/secrets/jwt_secret

   # Admin JWT secret (generate a secure random string)
   openssl rand -base64 32 | sudo tee /etc/coldnb/secrets/admin_jwt_secret

   # Stripe keys
   echo "sk_live_..." | sudo tee /etc/coldnb/secrets/stripe_secret_key
   echo "whsec_..." | sudo tee /etc/coldnb/secrets/stripe_webhook_secret

   # Brevo API key
   echo "xkeysib-..." | sudo tee /etc/coldnb/secrets/brevo_api_key

   # Secure the secrets
   sudo chmod 600 /etc/coldnb/secrets/*
   ```

3. Edit `/etc/coldnb/server.conf` with your settings.

## Database Setup

1. Create the database and user:
   ```bash
   sudo -u postgres psql
   CREATE USER coldnb WITH PASSWORD 'your_password';
   CREATE DATABASE coldnb OWNER coldnb;
   \c coldnb
   CREATE EXTENSION "uuid-ossp";
   \q
   ```

2. Run migrations:
   ```bash
   psql -U coldnb -d coldnb -f sql/001_initial_schema.sql
   ```

3. Create admin user:
   ```bash
   # Build the password generator
   make tools

   # Generate a password hash
   ./build/bin/generate_admin_password 'YourSecurePassword'

   # Run the generated SQL INSERT statement
   ```

## Running

```bash
# Direct execution
./build/bin/coldnb-server

# With custom config
./build/bin/coldnb-server -c /path/to/server.conf

# As systemd service (after installation)
sudo systemctl start coldnb
sudo systemctl enable coldnb
```

## API Documentation

See `docs/api.md` for complete API documentation.

### Quick Reference

**Public Endpoints:**
- `GET /api/products` - List products
- `GET /api/products/:id` - Get product
- `GET /api/products/search?q=...` - Search products
- `POST /api/newsletter/subscribe` - Subscribe to newsletter
- `POST /api/contact` - Submit contact form

**Authenticated Endpoints (User):**
- `GET/POST/PUT/DELETE /api/cart/*` - Cart operations
- `GET/POST/DELETE /api/wishlist/*` - Wishlist operations
- `GET/PUT /api/user/profile` - User profile
- `GET/POST/PUT/DELETE /api/user/addresses/*` - Addresses
- `POST /api/orders` - Create order
- `GET /api/orders` - Order history

**Payment Endpoints:**
- `POST /api/payments/create-intent` - Create payment
- `POST /api/payments/pix` - Generate PIX code
- `POST /api/webhooks/stripe` - Stripe webhook

**Admin Endpoints:**
- `POST /api/admin/login` - Admin login
- `GET/PUT /api/admin/users/*` - User management
- `GET/PUT /api/admin/orders/*` - Order management
- `GET/POST/PUT/DELETE /api/admin/products/*` - Product CRUD
- `GET /api/admin/analytics/*` - Analytics

## License

Proprietary - All rights reserved
