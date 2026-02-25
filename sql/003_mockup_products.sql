-- Coldnb Mockup Products for Testing
-- 30 random jewelry products for testing shopping cart, checkout, etc.
-- Categories IDs: 7=Necklaces, 8=Earrings, 9=Bracelets, 10=Rings, 11=Anklets, 12=Pendants

INSERT INTO products (
    name, slug, description, short_description, sku, price, compare_at_price,
    cost_price, category_id, brand, stock_quantity, is_active, is_featured,
    is_new, is_sale, meta_title, meta_description
) VALUES
    -- Product 1-5 (Mixed categories with random attributes)
    ('Golden Heart Necklace', 'golden-heart-necklace-001', 'Beautiful golden heart pendant with delicate chain', 'Elegant golden heart', 'SKU-001', 89.99, 129.99, 45.00, 7, 'Coldnb Jewels', 15, true, true, true, true, 'Golden Heart Necklace', 'Premium golden heart necklace'),
    ('Silver Pearl Earrings', 'silver-pearl-earrings-002', 'Classic pearl earrings with silver setting', 'Timeless pearl studs', 'SKU-002', 45.50, 65.00, 20.00, 8, 'Coldnb Jewels', 28, true, true, false, false, 'Silver Pearl Earrings', 'Elegant silver pearl earrings'),
    ('Diamond Bracelet', 'diamond-bracelet-003', 'Sparkling diamond tennis bracelet', 'Luxury diamond bracelet', 'SKU-003', 299.99, 399.99, 150.00, 9, 'Luxury Line', 8, true, true, false, true, 'Diamond Bracelet', 'Premium diamond tennis bracelet'),
    ('Rose Gold Ring', 'rose-gold-ring-004', 'Modern rose gold band ring', 'Contemporary rose gold', 'SKU-004', 78.75, 99.99, 40.00, 10, 'Coldnb Jewels', 42, true, false, false, false, 'Rose Gold Ring', 'Stylish rose gold ring'),
    ('Crystal Anklet', 'crystal-anklet-005', 'Delicate crystal beaded anklet', 'Sparkling crystal design', 'SKU-005', 34.99, 49.99, 15.00, 11, 'Coldnb Jewels', 55, true, false, true, false, 'Crystal Anklet', 'Beautiful crystal anklet'),

    -- Product 6-10
    ('Emerald Pendant', 'emerald-pendant-006', 'Genuine emerald stone pendant with gold chain', 'Premium emerald gem', 'SKU-006', 156.50, 199.99, 80.00, 12, 'Luxury Line', 12, true, true, false, true, 'Emerald Pendant', 'Exquisite emerald pendant'),
    ('Turquoise Drop Earrings', 'turquoise-drop-earrings-007', 'Vibrant turquoise stone drop earrings', 'Colorful turquoise drops', 'SKU-007', 52.00, 75.00, 25.00, 8, 'Coldnb Jewels', 34, true, false, true, false, 'Turquoise Drop Earrings', 'Vibrant turquoise earrings'),
    ('Moonstone Bracelet', 'moonstone-bracelet-008', 'Iridescent moonstone stretch bracelet', 'Magical moonstone', 'SKU-008', 67.25, 95.00, 32.00, 9, 'Coldnb Jewels', 21, true, true, false, false, 'Moonstone Bracelet', 'Beautiful moonstone bracelet'),
    ('Sapphire Ring', 'sapphire-ring-009', 'Deep blue sapphire gemstone ring', 'Royal blue sapphire', 'SKU-009', 189.99, 269.99, 95.00, 10, 'Luxury Line', 9, true, true, false, true, 'Sapphire Ring', 'Elegant sapphire ring'),
    ('Gold Chain Anklet', 'gold-chain-anklet-010', 'Classic gold plated chain anklet', 'Minimalist gold chain', 'SKU-010', 38.50, 55.00, 18.00, 11, 'Coldnb Jewels', 48, true, false, true, false, 'Gold Chain Anklet', 'Classic gold anklet'),

    -- Product 11-15
    ('Amethyst Necklace', 'amethyst-necklace-011', 'Raw amethyst crystal necklace with leather cord', 'Natural amethyst stone', 'SKU-011', 62.75, 89.99, 30.00, 7, 'Coldnb Jewels', 26, true, false, false, false, 'Amethyst Necklace', 'Natural amethyst necklace'),
    ('Pearl Stud Earrings', 'pearl-stud-earrings-012', 'Freshwater pearl stud earrings', 'Classic pearl studs', 'SKU-012', 43.00, 65.00, 20.00, 8, 'Coldnb Jewels', 52, true, true, true, false, 'Pearl Stud Earrings', 'Timeless pearl studs'),
    ('Ruby Bracelet', 'ruby-bracelet-013', 'Crimson ruby gemstone bracelet', 'Vibrant ruby stones', 'SKU-013', 234.50, 329.99, 120.00, 9, 'Luxury Line', 11, true, true, false, true, 'Ruby Bracelet', 'Premium ruby bracelet'),
    ('Opal Ring', 'opal-ring-014', 'Multicolor opal statement ring', 'Colorful opal gemstone', 'SKU-014', 124.99, 175.00, 60.00, 10, 'Coldnb Jewels', 18, true, false, true, false, 'Opal Ring', 'Beautiful opal ring'),
    ('Beaded Anklet', 'beaded-anklet-015', 'Colorful wooden bead anklet', 'Boho beaded design', 'SKU-015', 28.75, 45.00, 12.00, 11, 'Coldnb Jewels', 63, true, false, false, false, 'Beaded Anklet', 'Bohemian beaded anklet'),

    -- Product 16-20
    ('Aquamarine Pendant', 'aquamarine-pendant-016', 'Serene aquamarine stone pendant', 'Ocean blue aquamarine', 'SKU-016', 98.50, 145.00, 48.00, 12, 'Coldnb Jewels', 19, true, true, false, false, 'Aquamarine Pendant', 'Beautiful aquamarine pendant'),
    ('Diamond Stud Earrings', 'diamond-stud-earrings-017', 'Classic diamond stud earrings 0.5ct', 'Timeless diamonds', 'SKU-017', 445.00, 599.99, 225.00, 8, 'Luxury Line', 6, true, true, false, true, 'Diamond Stud Earrings', 'Premium diamond studs'),
    ('Coral Bracelet', 'coral-bracelet-018', 'Pink coral beaded bracelet', 'Tropical coral design', 'SKU-018', 54.25, 79.99, 25.00, 9, 'Coldnb Jewels', 38, true, false, true, false, 'Coral Bracelet', 'Vibrant coral bracelet'),
    ('Citrine Ring', 'citrine-ring-019', 'Sunny citrine quartz ring', 'Golden citrine stone', 'SKU-019', 85.99, 125.00, 42.00, 10, 'Coldnb Jewels', 31, true, true, false, false, 'Citrine Ring', 'Beautiful citrine ring'),
    ('Silver Chain Anklet', 'silver-chain-anklet-020', 'Delicate silver plated ankle chain', 'Simple silver chain', 'SKU-020', 32.50, 49.99, 15.00, 11, 'Coldnb Jewels', 71, true, false, false, false, 'Silver Chain Anklet', 'Elegant silver anklet'),

    -- Product 21-25
    ('Garnet Necklace', 'garnet-necklace-021', 'Deep red garnet stone necklace', 'Rich garnet pendant', 'SKU-021', 72.00, 105.00, 35.00, 7, 'Coldnb Jewels', 22, true, false, true, false, 'Garnet Necklace', 'Elegant garnet necklace'),
    ('Crystal Chandelier Earrings', 'crystal-chandelier-earrings-022', 'Sparkling crystal drop earrings', 'Glamorous chandelier style', 'SKU-022', 68.50, 95.00, 32.00, 8, 'Coldnb Jewels', 29, true, true, false, false, 'Crystal Chandelier Earrings', 'Elegant crystal earrings'),
    ('Topaz Bracelet', 'topaz-bracelet-023', 'Warm golden topaz bracelet', 'Glowing topaz stones', 'SKU-023', 94.75, 139.99, 47.00, 9, 'Coldnb Jewels', 25, true, true, true, false, 'Topaz Bracelet', 'Beautiful topaz bracelet'),
    ('Tanzanite Ring', 'tanzanite-ring-024', 'Blue-violet tanzanite gemstone ring', 'Rare tanzanite stone', 'SKU-024', 267.50, 399.99, 135.00, 10, 'Luxury Line', 7, true, true, false, true, 'Tanzanite Ring', 'Exquisite tanzanite ring'),
    ('Shell Anklet', 'shell-anklet-025', 'Seashell charm ankle bracelet', 'Beach-inspired design', 'SKU-025', 24.99, 39.99, 10.00, 11, 'Coldnb Jewels', 85, true, false, true, false, 'Shell Anklet', 'Tropical shell anklet'),

    -- Product 26-30
    ('Jade Pendant', 'jade-pendant-026', 'Traditional carved jade pendant', 'Authentic jade stone', 'SKU-026', 142.00, 199.99, 70.00, 12, 'Coldnb Jewels', 14, true, true, false, true, 'Jade Pendant', 'Traditional jade pendant'),
    ('Marcasite Earrings', 'marcasite-earrings-027', 'Vintage style marcasite earrings', 'Antique marcasite design', 'SKU-027', 56.75, 85.00, 27.00, 8, 'Coldnb Jewels', 33, true, false, false, false, 'Marcasite Earrings', 'Vintage marcasite earrings'),
    ('Labradorite Bracelet', 'labradorite-bracelet-028', 'Iridescent labradorite stone bracelet', 'Mystical labradorite', 'SKU-028', 79.50, 115.00, 39.00, 9, 'Coldnb Jewels', 27, true, true, true, false, 'Labradorite Bracelet', 'Magical labradorite bracelet'),
    ('Peridot Ring', 'peridot-ring-029', 'Fresh green peridot gemstone ring', 'Vibrant peridot stone', 'SKU-029', 103.25, 149.99, 51.00, 10, 'Coldnb Jewels', 20, true, false, false, false, 'Peridot Ring', 'Beautiful peridot ring'),
    ('Gold Tassel Anklet', 'gold-tassel-anklet-030', 'Gold plated anklet with tassel charm', 'Boho tassel design', 'SKU-030', 41.50, 65.00, 20.00, 11, 'Coldnb Jewels', 44, true, true, false, false, 'Gold Tassel Anklet', 'Stylish gold tassel anklet');

-- Insert product images - reusing the 5 images from local folder
-- Each product gets 1-2 images

INSERT INTO product_images (product_id, url, alt_text, sort_order, is_primary)
SELECT
    p.id,
    CASE (p.id % 5)
        WHEN 0 THEN '/images/brinco-bombe-de-ouro-18k-grande-ab7e9ed0223498a79e0cbdba2269987b.jpg'
        WHEN 1 THEN '/images/brinco-de-argola-flor-preta-8kcijra3zv.webp'
        WHEN 2 THEN '/images/images (1).jpeg'
        WHEN 3 THEN '/images/images.jpeg'
        WHEN 4 THEN '/images/pulseira_feminina_prata_925_flor_branca_1_20250926120505_1e414f07b3a6.webp'
    END,
    p.name || ' - Main Image',
    0,
    true
FROM products p
WHERE p.sku LIKE 'SKU-%'
ORDER BY p.id;

-- Add secondary images for some products (every other product)
INSERT INTO product_images (product_id, url, alt_text, sort_order, is_primary)
SELECT
    p.id,
    CASE ((p.id * 3) % 5)
        WHEN 0 THEN '/images/brinco-bombe-de-ouro-18k-grande-ab7e9ed0223498a79e0cbdba2269987b.jpg'
        WHEN 1 THEN '/images/brinco-de-argola-flor-preta-8kcijra3zv.webp'
        WHEN 2 THEN '/images/images (1).jpeg'
        WHEN 3 THEN '/images/images.jpeg'
        WHEN 4 THEN '/images/pulseira_feminina_prata_925_flor_branca_1_20250926120505_1e414f07b3a6.webp'
    END,
    p.name || ' - Alternate View',
    1,
    false
FROM products p
WHERE p.sku LIKE 'SKU-%' AND (p.id % 2) = 0
ORDER BY p.id;

-- Verify insertion
SELECT 'Insertion Complete!' as status;
SELECT COUNT(*) as total_products FROM products WHERE sku LIKE 'SKU-%';
SELECT COUNT(*) as total_images FROM product_images WHERE product_id IN (SELECT id FROM products WHERE sku LIKE 'SKU-%');

-- Show sample products
SELECT id, name, price, stock_quantity, brand, category_id FROM products WHERE sku LIKE 'SKU-%' ORDER BY id LIMIT 10;
