-- Migration 008: Abandoned cart recovery tracking + Loyalty rewards program
-- Requires: 001_initial_schema.sql (users, cart_items, products tables)

-- Track abandoned cart recovery emails sent
CREATE TABLE IF NOT EXISTS abandoned_cart_emails (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    email VARCHAR(255) NOT NULL,
    cart_item_count INTEGER NOT NULL DEFAULT 0,
    cart_total NUMERIC(10,2) NOT NULL DEFAULT 0,
    sent_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    clicked_at TIMESTAMP WITH TIME ZONE,
    converted_at TIMESTAMP WITH TIME ZONE
);

CREATE INDEX idx_abandoned_cart_emails_user ON abandoned_cart_emails(user_id);
CREATE INDEX idx_abandoned_cart_emails_sent ON abandoned_cart_emails(sent_at);

-- Loyalty points ledger
CREATE TABLE IF NOT EXISTS loyalty_points (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    points INTEGER NOT NULL,
    reason VARCHAR(100) NOT NULL,
    reference_id VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_loyalty_points_user ON loyalty_points(user_id);

-- Loyalty rewards catalog
CREATE TABLE IF NOT EXISTS loyalty_rewards (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    points_cost INTEGER NOT NULL,
    reward_type VARCHAR(50) NOT NULL DEFAULT 'discount',
    reward_value NUMERIC(10,2) NOT NULL,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TRIGGER update_loyalty_rewards_updated_at
    BEFORE UPDATE ON loyalty_rewards
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at();

-- Loyalty redemptions
CREATE TABLE IF NOT EXISTS loyalty_redemptions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    reward_id INTEGER NOT NULL REFERENCES loyalty_rewards(id),
    points_spent INTEGER NOT NULL,
    discount_code VARCHAR(50),
    order_id UUID REFERENCES orders(id),
    redeemed_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_loyalty_redemptions_user ON loyalty_redemptions(user_id);
