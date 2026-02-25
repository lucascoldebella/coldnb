-- Migration: Homepage Content Management
-- Description: Tables for admin-controlled homepage hero slides, banners, sections, and campaigns

-- =============================================================================
-- HOMEPAGE HERO SLIDES
-- =============================================================================
CREATE TABLE IF NOT EXISTS homepage_hero_slides (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255),
    subtitle VARCHAR(255),
    button_text VARCHAR(100),
    button_link VARCHAR(500),
    image_url VARCHAR(500),
    image_alt VARCHAR(255),
    product_id INTEGER REFERENCES products(id) ON DELETE SET NULL,
    category_id INTEGER REFERENCES categories(id) ON DELETE SET NULL,
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    starts_at TIMESTAMP WITH TIME ZONE,
    ends_at TIMESTAMP WITH TIME ZONE,
    created_by UUID REFERENCES admin_users(id) ON DELETE SET NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_hero_slides_active ON homepage_hero_slides(is_active, sort_order);
CREATE INDEX idx_hero_slides_schedule ON homepage_hero_slides(starts_at, ends_at)
    WHERE starts_at IS NOT NULL OR ends_at IS NOT NULL;

CREATE TRIGGER update_homepage_hero_slides_updated_at
    BEFORE UPDATE ON homepage_hero_slides
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- HOMEPAGE BANNERS
-- =============================================================================
CREATE TABLE IF NOT EXISTS homepage_banners (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255),
    subtitle VARCHAR(255),
    button_text VARCHAR(100),
    button_link VARCHAR(500),
    image_url VARCHAR(500),
    image_alt VARCHAR(255),
    banner_type VARCHAR(20) NOT NULL DEFAULT 'collection'
        CHECK (banner_type IN ('collection', 'countdown')),
    text_color VARCHAR(10) DEFAULT 'dark'
        CHECK (text_color IN ('dark', 'white')),
    position VARCHAR(10) DEFAULT 'left'
        CHECK (position IN ('left', 'right')),
    countdown_end_at TIMESTAMP WITH TIME ZONE,
    discount_label VARCHAR(100),
    product_id INTEGER REFERENCES products(id) ON DELETE SET NULL,
    category_id INTEGER REFERENCES categories(id) ON DELETE SET NULL,
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    starts_at TIMESTAMP WITH TIME ZONE,
    ends_at TIMESTAMP WITH TIME ZONE,
    created_by UUID REFERENCES admin_users(id) ON DELETE SET NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_banners_type_active ON homepage_banners(banner_type, is_active, sort_order);
CREATE INDEX idx_banners_schedule ON homepage_banners(starts_at, ends_at)
    WHERE starts_at IS NOT NULL OR ends_at IS NOT NULL;

CREATE TRIGGER update_homepage_banners_updated_at
    BEFORE UPDATE ON homepage_banners
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- HOMEPAGE SECTIONS
-- =============================================================================
CREATE TABLE IF NOT EXISTS homepage_sections (
    id SERIAL PRIMARY KEY,
    section_key VARCHAR(50) NOT NULL UNIQUE,
    title VARCHAR(255),
    subtitle VARCHAR(500),
    source_type VARCHAR(20) DEFAULT 'manual'
        CHECK (source_type IN ('manual', 'featured', 'new', 'bestseller', 'on_sale', 'category')),
    category_id INTEGER REFERENCES categories(id) ON DELETE SET NULL,
    max_items INTEGER DEFAULT 8,
    config JSONB DEFAULT '{}',
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TRIGGER update_homepage_sections_updated_at
    BEFORE UPDATE ON homepage_sections
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- HOMEPAGE SECTION PRODUCTS (manual picks)
-- =============================================================================
CREATE TABLE IF NOT EXISTS homepage_section_products (
    id SERIAL PRIMARY KEY,
    section_id INTEGER NOT NULL REFERENCES homepage_sections(id) ON DELETE CASCADE,
    product_id INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,
    tab_name VARCHAR(100),
    sort_order INTEGER DEFAULT 0,
    UNIQUE(section_id, product_id, tab_name)
);

CREATE INDEX idx_section_products_section ON homepage_section_products(section_id, sort_order);

-- =============================================================================
-- HOMEPAGE CAMPAIGNS
-- =============================================================================
CREATE TABLE IF NOT EXISTS homepage_campaigns (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    starts_at TIMESTAMP WITH TIME ZONE,
    ends_at TIMESTAMP WITH TIME ZONE,
    is_active BOOLEAN DEFAULT false,
    hero_slide_ids INTEGER[] DEFAULT '{}',
    banner_ids INTEGER[] DEFAULT '{}',
    created_by UUID REFERENCES admin_users(id) ON DELETE SET NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_campaigns_active ON homepage_campaigns(is_active, starts_at, ends_at);

CREATE TRIGGER update_homepage_campaigns_updated_at
    BEFORE UPDATE ON homepage_campaigns
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- SEED DATA: Migrate current hardcoded homepage content
-- =============================================================================

-- Hero slides (from data/heroSlides.js: slides array)
INSERT INTO homepage_hero_slides (title, subtitle, button_text, button_link, image_url, image_alt, sort_order, is_active) VALUES
    ('Flash Sale Madness', 'BIKINIS & SWIMSUITS', 'Explore Collection', '/shop-default-grid', '/images/slider/slider-women1.jpg', 'fashion-slideshow', 0, true),
    ('Find Your\nSignature Style', 'BIKINIS & SWIMSUITS', 'Explore Collection', '/shop-default-grid', '/images/slider/slider-women2.jpg', 'fashion-slideshow', 1, true)
ON CONFLICT DO NOTHING;

-- Collection banners (from BannerCollection.jsx)
INSERT INTO homepage_banners (title, subtitle, button_text, button_link, image_url, image_alt, banner_type, text_color, position, sort_order, is_active) VALUES
    ('Crossbody bag', 'From beach to party: Perfect styles for every occasion.', 'Shop Now', '/shop-collection', '/images/collections/banner-collection/banner-cls1.jpg', 'banner-cls', 'collection', 'dark', 'left', 0, true),
    ('Capsule Collection', 'Reserved for special occasions', 'Shop Now', '/shop-collection', '/images/collections/banner-collection/banner-cls2.jpg', 'banner-cls', 'collection', 'white', 'right', 1, true)
ON CONFLICT DO NOTHING;

-- Countdown banner (from BannerCountdown.jsx)
INSERT INTO homepage_banners (title, subtitle, discount_label, button_text, button_link, image_url, image_alt, banner_type, countdown_end_at, sort_order, is_active) VALUES
    ('Limited-Time Deals On!', 'Up to 50% Off Selected Styles. Don''t Miss Out.', 'Up to 50% Off', 'Shop Now', '/shop-default-grid', '/images/banner/img-countdown1.png', 'banner', 'countdown', NOW() + INTERVAL '12 days', 0, true)
ON CONFLICT DO NOTHING;

-- Default homepage sections
INSERT INTO homepage_sections (section_key, title, subtitle, source_type, config, sort_order, is_active) VALUES
    ('products_tabbed', 'Our Products', NULL, 'manual', '{"tabs": [{"name": "New Arrivals", "source": "new"}, {"name": "Best Sellers", "source": "bestseller"}, {"name": "On Sale", "source": "on_sale"}]}', 0, true),
    ('testimonials', 'Happy Clients', NULL, 'featured', '{}', 1, true),
    ('shopgram', 'Shop Gram', 'Inspire and let yourself be inspired, from one unique style to another.', 'featured', '{}', 2, true)
ON CONFLICT (section_key) DO NOTHING;

-- Add manage_homepage permission to permission documentation
COMMENT ON TABLE homepage_hero_slides IS 'Homepage hero carousel slides. Permission: manage_homepage';
COMMENT ON TABLE homepage_banners IS 'Homepage collection and countdown banners. Permission: manage_homepage';
COMMENT ON TABLE homepage_sections IS 'Configurable homepage product showcase sections. Permission: manage_homepage';
COMMENT ON TABLE homepage_campaigns IS 'Coordinated marketing campaigns bundling slides and banners. Permission: manage_homepage';
