-- Migration: Navigation Menu Management
-- Description: Tables for admin-controlled navigation menus, groups, and items

-- =============================================================================
-- NAVIGATION MENUS (top-level: Inicio, Loja, Produtos, Blog, Paginas)
-- =============================================================================
CREATE TABLE IF NOT EXISTS navigation_menus (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    slug VARCHAR(100) NOT NULL UNIQUE,
    menu_type VARCHAR(20) NOT NULL DEFAULT 'simple'
        CHECK (menu_type IN ('mega_grid', 'mega_columns', 'simple')),
    show_products BOOLEAN DEFAULT false,
    products_count INTEGER DEFAULT 4,
    banner_image_url VARCHAR(500),
    banner_link VARCHAR(500),
    banner_title VARCHAR(255),
    translation_key VARCHAR(100),
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_nav_menus_active ON navigation_menus(is_active, sort_order);

CREATE TRIGGER update_navigation_menus_updated_at
    BEFORE UPDATE ON navigation_menus
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- NAVIGATION GROUPS (columns/sections within a menu)
-- =============================================================================
CREATE TABLE IF NOT EXISTS navigation_groups (
    id SERIAL PRIMARY KEY,
    menu_id INTEGER NOT NULL REFERENCES navigation_menus(id) ON DELETE CASCADE,
    title VARCHAR(255),
    translation_key VARCHAR(100),
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_nav_groups_menu ON navigation_groups(menu_id, is_active, sort_order);

CREATE TRIGGER update_navigation_groups_updated_at
    BEFORE UPDATE ON navigation_groups
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- NAVIGATION ITEMS (individual links within a group)
-- =============================================================================
CREATE TABLE IF NOT EXISTS navigation_items (
    id SERIAL PRIMARY KEY,
    group_id INTEGER NOT NULL REFERENCES navigation_groups(id) ON DELETE CASCADE,
    label VARCHAR(255) NOT NULL,
    href VARCHAR(500) NOT NULL,
    image_url VARCHAR(500),
    image_alt VARCHAR(255),
    badge VARCHAR(50),
    sort_order INTEGER DEFAULT 0,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_nav_items_group ON navigation_items(group_id, is_active, sort_order);

CREATE TRIGGER update_navigation_items_updated_at
    BEFORE UPDATE ON navigation_items
    FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- =============================================================================
-- SEED DATA
-- =============================================================================

-- Menu 1: Inicio (mega_grid - image cards)
INSERT INTO navigation_menus (name, slug, menu_type, translation_key, sort_order)
VALUES ('Inicio', 'inicio', 'mega_grid', 'nav.home', 1);

-- Menu 2: Loja (mega_columns with product carousel)
INSERT INTO navigation_menus (name, slug, menu_type, show_products, products_count, translation_key, sort_order)
VALUES ('Loja', 'loja', 'mega_columns', true, 4, 'nav.shop', 2);

-- Menu 3: Produtos (mega_columns with banner)
INSERT INTO navigation_menus (name, slug, menu_type, banner_image_url, banner_link, banner_title, translation_key, sort_order)
VALUES ('Produtos', 'produtos', 'mega_columns',
        '/images/collections/cls-header.jpg', '/shop-collection', 'nav.bestSeller',
        'nav.products', 3);

-- Menu 4: Blog (simple dropdown)
INSERT INTO navigation_menus (name, slug, menu_type, translation_key, sort_order)
VALUES ('Blog', 'blog', 'simple', 'nav.blog', 4);

-- Menu 5: Paginas (simple dropdown)
INSERT INTO navigation_menus (name, slug, menu_type, translation_key, sort_order)
VALUES ('Paginas', 'paginas', 'simple', 'nav.pages', 5);

-- =============================================================================
-- INICIO groups & items (mega_grid: single group, image items)
-- =============================================================================
INSERT INTO navigation_groups (menu_id, title, sort_order)
VALUES (1, NULL, 1);

INSERT INTO navigation_items (group_id, label, href, image_url, image_alt, sort_order) VALUES
(1, 'Jewelry ElegantGems', '/home-jewelry-01', '/images/demo/home-jewelry.jpg', 'home-jewelry-elegantGems', 1),
(1, 'Jewelry GlitterGlam', '/home-jewelry-02', '/images/demo/home-jewelry2.jpg', 'home-jewelry-glitterGlam', 2),
(1, 'Jewelry Rings', '/home-jewelry-rings', '/images/demo/home-jewelry-rings.jpg', 'home-jewelry-rings', 3);

-- =============================================================================
-- LOJA groups & items (mega_columns: 4 groups)
-- =============================================================================
-- Group: Shop Layout
INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order)
VALUES (2, 'Shop Layout', 'nav.shopLayout', 1);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(2, 'Default Grid', '/shop-default-grid', 1),
(2, 'Default List', '/shop-default-list', 2),
(2, 'Full Width List', '/shop-fullwidth-list', 3),
(2, 'Full Width Grid', '/shop-fullwidth-grid', 4),
(2, 'Left Sidebar', '/shop-left-sidebar', 5),
(2, 'Right Sidebar', '/shop-right-sidebar', 6),
(2, 'Filter Dropdown', '/shop-filter-dropdown', 7),
(2, 'Filter Canvas', '/shop-filter-canvas', 8);

-- Group: Shop Features
INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order)
VALUES (2, 'Shop Features', 'nav.shopFeatures', 2);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(3, 'Categories Top 1', '/shop-categories-top', 1),
(3, 'Categories Top 2', '/shop-categories-top-02', 2),
(3, 'Shop Collection', '/shop-collection', 3),
(3, 'Breadcrumb IMG', '/shop-breadcrumb-img', 4),
(3, 'Breadcrumb Left', '/shop-breadcrumb-left', 5),
(3, 'Breadcrumb BG', '/shop-breadcrumb-background', 6),
(3, 'Load Button', '/shop-load-button', 7),
(3, 'Pagination', '/shop-pagination', 8),
(3, 'Infinite Scrolling', '/shop-infinite-scrolling', 9);

-- Group: Products Hover
INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order)
VALUES (2, 'Products Hover', 'nav.productsHover', 3);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(4, 'Product Style 1', '/product-style-01', 1),
(4, 'Product Style 2', '/product-style-02', 2),
(4, 'Product Style 3', '/product-style-03', 3),
(4, 'Product Style 4', '/product-style-04', 4),
(4, 'Product Style 5', '/product-style-05', 5),
(4, 'Product Style 6', '/product-style-06', 6),
(4, 'Product Style 7', '/product-style-07', 7);

-- Group: My Pages
INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order)
VALUES (2, 'My Pages', 'nav.myPages', 4);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(5, 'Wish List', '/wish-list', 1),
(5, 'Search Result', '/search-result', 2),
(5, 'Shopping Cart', '/shopping-cart', 3),
(5, 'Login/Register', '/login', 4),
(5, 'Forget Password', '/forget-password', 5),
(5, 'Order Tracking', '/order-tracking', 6),
(5, 'My Account', '/my-account', 7);

-- =============================================================================
-- PRODUTOS groups & items (mega_columns: 1 group + banner)
-- =============================================================================
INSERT INTO navigation_groups (menu_id, title, translation_key, sort_order)
VALUES (3, 'Products Layout', 'nav.productsLayout', 1);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(6, 'Product Default', '/product-detail/1', 1);

-- =============================================================================
-- BLOG items (simple: single group)
-- =============================================================================
INSERT INTO navigation_groups (menu_id, title, sort_order)
VALUES (4, NULL, 1);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(7, 'Blog Default', '/blog-default', 1),
(7, 'Blog List', '/blog-list', 2),
(7, 'Blog Grid', '/blog-grid', 3),
(7, 'Blog Detail 1', '/blog-detail/1', 4),
(7, 'Blog Detail 2', '/blog-detail-02/2', 5);

-- =============================================================================
-- PAGINAS items (simple: single group)
-- =============================================================================
INSERT INTO navigation_groups (menu_id, title, sort_order)
VALUES (5, NULL, 1);

INSERT INTO navigation_items (group_id, label, href, sort_order) VALUES
(8, 'About Us', '/about-us', 1),
(8, 'Contato', '/contact', 2),
(8, 'FAQs', '/FAQs', 3),
(8, 'Terms Of Use', '/term-of-use', 4),
(8, 'Coming Soon', '/coming-soon', 5),
(8, 'Customer Feedbacks', '/customer-feedback', 6);
