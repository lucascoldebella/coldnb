-- Migration: Order Tracking Support
-- Description: Add tracking_number, carrier, and estimated_delivery to orders table

-- =============================================================================
-- ADD TRACKING COLUMNS TO ORDERS
-- =============================================================================

ALTER TABLE orders ADD COLUMN IF NOT EXISTS tracking_number VARCHAR(255);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS carrier VARCHAR(100);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS estimated_delivery_date DATE;

CREATE INDEX IF NOT EXISTS idx_orders_tracking ON orders(tracking_number) WHERE tracking_number IS NOT NULL;
