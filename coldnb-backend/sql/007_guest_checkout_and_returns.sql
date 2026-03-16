-- Migration 007: Guest checkout support + Returns workflow
-- Apply after 006_order_tracking.sql

-- -------------------------------------------------------
-- Guest checkout: add contact fields to orders table
-- -------------------------------------------------------
ALTER TABLE orders
    ADD COLUMN IF NOT EXISTS guest_email VARCHAR(255),
    ADD COLUMN IF NOT EXISTS guest_name  VARCHAR(255);

-- Index for looking up guest orders by email
CREATE INDEX IF NOT EXISTS idx_orders_guest_email ON orders(guest_email)
    WHERE guest_email IS NOT NULL;

-- -------------------------------------------------------
-- Returns table
-- -------------------------------------------------------
CREATE TABLE IF NOT EXISTS order_returns (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    order_id        UUID NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
    order_item_id   UUID REFERENCES order_items(id) ON DELETE SET NULL,
    reason          VARCHAR(100) NOT NULL,   -- 'defective', 'wrong_item', 'changed_mind', 'other'
    description     TEXT,
    status          VARCHAR(50) DEFAULT 'requested',
                    -- requested -> under_review -> approved -> rejected -> refunded
    admin_notes     TEXT,
    refund_amount   DECIMAL(10, 2),
    created_at      TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at      TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    resolved_at     TIMESTAMP WITH TIME ZONE
);

CREATE INDEX IF NOT EXISTS idx_order_returns_order ON order_returns(order_id);
CREATE INDEX IF NOT EXISTS idx_order_returns_status ON order_returns(status);

-- Auto-update updated_at trigger
CREATE TRIGGER update_order_returns_updated_at
    BEFORE UPDATE ON order_returns
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();
