---
applies_to: ["coldnb-backend/sql/**", "**/*.sql"]
---

# Database Migration Rules

## Migration File Conventions
- Files live in `coldnb-backend/sql/`
- Naming: `00N_description.sql` (sequential, 3-digit zero-padded)
- Apply in order: 001 → 002 → 003 → 004 → 005
- **NEVER** modify an already-applied migration — create a new one instead

## Current Migrations (Applied Order)
1. `001_initial_schema.sql` — Core tables: users, admin_users, categories, products, product_images, product_colors, product_sizes, product_tags, user_addresses, cart_items, wishlist_items, discount_codes, orders, order_items, order_history, newsletter_subscribers, contact_submissions, analytics_page_views, analytics_product_views, admin_sessions
2. `002_admin_permissions.sql` — Admin roles and granular permissions
3. `003_homepage_content.sql` — Dynamic homepage content management tables
4. `004_shipping_zones.sql` — Shipping zones, methods, and rules
5. `005_navigation_menus.sql` — Dynamic navigation menu tables

## Rules
- **NEVER run migrations without explicit human confirmation**
- Use `IF NOT EXISTS` on `CREATE TABLE` to make migrations idempotent
- Use `ON CONFLICT DO NOTHING` on seed data inserts
- Always add `updated_at` triggers for tables with that column (use existing `update_updated_at()` function)
- Snapshot critical order data: `order_items` stores `product_name`, `product_sku`, `product_image` at time of purchase — never rely on JOIN to products table for historical order data

## Admin User Setup (NEVER Default Password)
```bash
# Generate hash first
cd coldnb-backend
./build/bin/generate_admin_password 'YourSecurePassword'

# Then insert
psql -U coldnb -d coldnb <<EOF
INSERT INTO admin_users (username, email, password_hash, full_name, role)
VALUES ('admin', 'admin@coldnb.com', '\$PASTE_HASH_HERE', 'Admin', 'super_admin');
EOF
```

## Schema Reset (DESTRUCTIVE — data loss)
Only use in development:
```bash
psql -U coldnb -d coldnb -c "DROP SCHEMA public CASCADE; CREATE SCHEMA public; GRANT ALL ON SCHEMA public TO coldnb;"
# Then reapply all migrations in order
```

## Key Schema Notes
- `orders.shipping_*` fields: snapshot of address at order time — not a FK to `user_addresses`
- `order_items`: snapshot of product name/sku/image at order time — product can be deleted/renamed safely
- `users.supabase_id`: synced from Supabase auth, used for user lookup from JWT claims
- `admin_users`: completely separate table, no relation to `users` table
- `discount_codes.usage_limit NULL` means unlimited usage
