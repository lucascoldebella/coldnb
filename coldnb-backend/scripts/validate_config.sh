#!/bin/bash
#
# Coldnb Backend - Configuration Validator
#
# Checks that all required secrets and configuration are properly set
# before starting the server.
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

ERRORS=0
WARNINGS=0

CONFIG_DIR="${CONFIG_DIR:-/etc/coldnb}"
SECRETS_DIR="$CONFIG_DIR/secrets"

echo "Coldnb Configuration Validator"
echo "==============================="
echo ""

# Check config file exists
if [ ! -f "$CONFIG_DIR/server.conf" ]; then
    echo -e "${RED}[ERROR]${NC} Config file not found: $CONFIG_DIR/server.conf"
    ERRORS=$((ERRORS + 1))
else
    echo -e "${GREEN}[OK]${NC} Config file exists"

    # Check for placeholder values
    if grep -q "YOUR_PROJECT_ID" "$CONFIG_DIR/server.conf"; then
        echo -e "${RED}[ERROR]${NC} Supabase project URL not configured"
        ERRORS=$((ERRORS + 1))
    fi

    if grep -q "YOUR_DOMAIN" "$CONFIG_DIR/server.conf"; then
        echo -e "${RED}[ERROR]${NC} CORS origins not configured"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Check secrets directory
if [ ! -d "$SECRETS_DIR" ]; then
    echo -e "${RED}[ERROR]${NC} Secrets directory not found: $SECRETS_DIR"
    ERRORS=$((ERRORS + 1))
else
    echo -e "${GREEN}[OK]${NC} Secrets directory exists"

    # Check permissions
    PERMS=$(stat -c %a "$SECRETS_DIR" 2>/dev/null || stat -f %Lp "$SECRETS_DIR" 2>/dev/null)
    if [ "$PERMS" != "700" ]; then
        echo -e "${YELLOW}[WARN]${NC} Secrets directory permissions should be 700 (current: $PERMS)"
        WARNINGS=$((WARNINGS + 1))
    fi
fi

# Check each secret file
SECRETS=(
    "db_password:Database password"
    "supabase_jwt_secret:Supabase JWT secret"
    "supabase_anon_key:Supabase anon key"
    "admin_jwt_secret:Admin JWT secret"
    "stripe_secret_key:Stripe secret key"
    "stripe_webhook_secret:Stripe webhook secret"
    "brevo_api_key:Brevo API key"
)

echo ""
echo "Checking secrets..."

for secret_info in "${SECRETS[@]}"; do
    secret_file="${secret_info%%:*}"
    secret_name="${secret_info#*:}"
    secret_path="$SECRETS_DIR/$secret_file"

    if [ ! -f "$secret_path" ]; then
        echo -e "${RED}[ERROR]${NC} Missing: $secret_name ($secret_file)"
        ERRORS=$((ERRORS + 1))
    else
        # Check if it's still a placeholder
        content=$(cat "$secret_path" 2>/dev/null)
        if [ "$content" = "REPLACE_WITH_ACTUAL_SECRET" ] || \
           [ "$content" = "your-supabase-jwt-secret-from-dashboard" ] || \
           [ "$content" = "your-supabase-anon-key-from-dashboard" ] || \
           [ "$content" = "change-this-to-a-secure-random-string-at-least-32-chars" ] || \
           [ "$content" = "sk_test_your_stripe_test_key_here" ] || \
           [ "$content" = "whsec_your_webhook_secret_here" ] || \
           [ "$content" = "your-brevo-api-key-here" ]; then
            echo -e "${RED}[ERROR]${NC} Placeholder value: $secret_name ($secret_file)"
            ERRORS=$((ERRORS + 1))
        else
            # Check permissions
            PERMS=$(stat -c %a "$secret_path" 2>/dev/null || stat -f %Lp "$secret_path" 2>/dev/null)
            if [ "$PERMS" != "600" ]; then
                echo -e "${YELLOW}[WARN]${NC} $secret_file permissions should be 600 (current: $PERMS)"
                WARNINGS=$((WARNINGS + 1))
            else
                echo -e "${GREEN}[OK]${NC} $secret_name"
            fi
        fi
    fi
done

# Check PostgreSQL connection
echo ""
echo "Checking PostgreSQL..."
if command -v psql &> /dev/null; then
    DB_PASS=$(cat "$SECRETS_DIR/db_password" 2>/dev/null)
    if PGPASSWORD="$DB_PASS" psql -h localhost -U coldnb -d coldnb -c "SELECT 1" &>/dev/null; then
        echo -e "${GREEN}[OK]${NC} PostgreSQL connection successful"

        # Check if schema is loaded
        TABLE_COUNT=$(PGPASSWORD="$DB_PASS" psql -h localhost -U coldnb -d coldnb -tAc "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = 'public'" 2>/dev/null)
        if [ "$TABLE_COUNT" -lt 10 ]; then
            echo -e "${YELLOW}[WARN]${NC} Database may not have schema loaded (only $TABLE_COUNT tables found)"
            WARNINGS=$((WARNINGS + 1))
        else
            echo -e "${GREEN}[OK]${NC} Database schema loaded ($TABLE_COUNT tables)"
        fi

        # Check for admin user
        ADMIN_COUNT=$(PGPASSWORD="$DB_PASS" psql -h localhost -U coldnb -d coldnb -tAc "SELECT COUNT(*) FROM admin_users" 2>/dev/null)
        if [ "$ADMIN_COUNT" -eq 0 ]; then
            echo -e "${YELLOW}[WARN]${NC} No admin users found - run generate_admin_password and INSERT"
            WARNINGS=$((WARNINGS + 1))
        else
            echo -e "${GREEN}[OK]${NC} Admin user(s) configured ($ADMIN_COUNT)"
        fi
    else
        echo -e "${RED}[ERROR]${NC} PostgreSQL connection failed"
        ERRORS=$((ERRORS + 1))
    fi
else
    echo -e "${YELLOW}[WARN]${NC} psql not found, skipping database check"
    WARNINGS=$((WARNINGS + 1))
fi

# Summary
echo ""
echo "==============================="
if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}All checks passed!${NC}"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}$WARNINGS warning(s), no errors${NC}"
    exit 0
else
    echo -e "${RED}$ERRORS error(s), $WARNINGS warning(s)${NC}"
    echo ""
    echo "Fix the errors above before starting the server."
    exit 1
fi
