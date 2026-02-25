-- Migration: Admin Permissions and Employee Extensions
-- Description: Add permissions, employee fields to admin_users table

-- Add new columns to admin_users table
ALTER TABLE admin_users
    ADD COLUMN IF NOT EXISTS permissions JSONB DEFAULT '{}',
    ADD COLUMN IF NOT EXISTS employee_id VARCHAR(50),
    ADD COLUMN IF NOT EXISTS cpf VARCHAR(14),
    ADD COLUMN IF NOT EXISTS photo_url VARCHAR(500),
    ADD COLUMN IF NOT EXISTS full_name VARCHAR(255),
    ADD COLUMN IF NOT EXISTS phone VARCHAR(20),
    ADD COLUMN IF NOT EXISTS created_by UUID REFERENCES admin_users(id),
    ADD COLUMN IF NOT EXISTS last_login TIMESTAMP WITH TIME ZONE;

-- Create index for faster permission queries
CREATE INDEX IF NOT EXISTS idx_admin_users_permissions ON admin_users USING GIN (permissions);

-- Create index for employee_id lookups
CREATE INDEX IF NOT EXISTS idx_admin_users_employee_id ON admin_users(employee_id) WHERE employee_id IS NOT NULL;

-- Create index for CPF lookups (unique constraint)
CREATE UNIQUE INDEX IF NOT EXISTS idx_admin_users_cpf ON admin_users(cpf) WHERE cpf IS NOT NULL;

-- Add comment explaining the permissions structure
COMMENT ON COLUMN admin_users.permissions IS 'JSON object containing granular permissions. Example: {"view_dashboard": true, "edit_products": true}';

-- Default permissions for different roles
-- super_admin: all permissions (bypasses checks in code)
-- admin: granular permissions stored in this column

-- Permission categories and their keys:
-- Dashboard: view_dashboard, customize_dashboard
-- Financial: view_financial, view_revenue, export_financial
-- Products: view_products, create_products, edit_products, delete_products, manage_categories, manage_inventory
-- Orders: view_orders, view_order_details, update_order_status, cancel_orders
-- Customers: view_customers, view_customer_details, edit_customers
-- Marketing: view_marketing, view_analytics, manage_discounts
-- Team: manage_team, create_employees, edit_employees, assign_permissions

-- Example: Create a default admin with standard permissions
-- INSERT INTO admin_users (username, email, password_hash, role, permissions, full_name)
-- VALUES (
--     'manager',
--     'manager@coldnb.com',
--     '$argon2id$...', -- Generate with: ./build/tools/generate_admin_password <password>
--     'admin',
--     '{
--         "view_dashboard": true,
--         "view_financial": true,
--         "view_products": true,
--         "edit_products": true,
--         "view_orders": true,
--         "update_order_status": true,
--         "view_customers": true,
--         "view_marketing": true
--     }'::jsonb,
--     'Store Manager'
-- );
