-- Coldnb Shipping Zones Migration
-- Distance-based shipping pricing from Campo Grande, MS

CREATE TABLE IF NOT EXISTS shipping_zones (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    min_distance_km INTEGER NOT NULL,
    max_distance_km INTEGER NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    estimated_days_min INTEGER DEFAULT 1,
    estimated_days_max INTEGER DEFAULT 3,
    is_active BOOLEAN DEFAULT true,
    sort_order INTEGER DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS cep_coordinates (
    cep_prefix VARCHAR(5) PRIMARY KEY,
    latitude DECIMAL(10,6) NOT NULL,
    longitude DECIMAL(10,6) NOT NULL,
    city VARCHAR(255),
    state VARCHAR(2)
);

-- Seed default shipping zones
INSERT INTO shipping_zones (name, min_distance_km, max_distance_km, price, estimated_days_min, estimated_days_max, sort_order)
VALUES
    ('Local (Campo Grande)', 0, 50, 10.00, 1, 2, 1),
    ('Regional (MS)', 50, 300, 18.00, 2, 4, 2),
    ('Interstate Short', 300, 800, 28.00, 4, 7, 3),
    ('Interstate Medium', 800, 1500, 38.00, 6, 10, 4),
    ('Interstate Long', 1500, 3000, 48.00, 8, 14, 5),
    ('Remote', 3000, 99999, 58.00, 10, 18, 6)
ON CONFLICT DO NOTHING;

-- Seed CEP coordinate data for major Brazilian regions
-- Origin: Campo Grande, MS (CEP 79xxx) at -20.4697, -54.6201
INSERT INTO cep_coordinates (cep_prefix, latitude, longitude, city, state)
VALUES
    -- Mato Grosso do Sul (79xxx)
    ('79000', -20.4697, -54.6201, 'Campo Grande', 'MS'),
    ('79100', -21.2049, -54.2523, 'Maracaju', 'MS'),
    ('79200', -22.2340, -54.8098, 'Dourados', 'MS'),
    ('79300', -20.4486, -55.7591, 'Corumba', 'MS'),
    ('79400', -21.6129, -55.1615, 'Ponta Pora', 'MS'),
    ('79500', -20.7846, -54.6157, 'Nova Andradina', 'MS'),
    ('79600', -22.3697, -53.8418, 'Navirai', 'MS'),
    ('79700', -21.7566, -54.7795, 'Rio Brilhante', 'MS'),
    ('79800', -20.4488, -54.5798, 'Terenos', 'MS'),
    ('79900', -19.0087, -57.6530, 'Ladario', 'MS'),

    -- Sao Paulo (01xxx-09xxx)
    ('01000', -23.5505, -46.6333, 'Sao Paulo Centro', 'SP'),
    ('01300', -23.5589, -46.6614, 'Sao Paulo', 'SP'),
    ('02000', -23.5024, -46.6269, 'Sao Paulo Norte', 'SP'),
    ('03000', -23.5467, -46.5830, 'Sao Paulo Leste', 'SP'),
    ('04000', -23.6103, -46.6417, 'Sao Paulo Sul', 'SP'),
    ('05000', -23.5316, -46.7119, 'Sao Paulo Oeste', 'SP'),
    ('06000', -23.5315, -46.7917, 'Osasco', 'SP'),
    ('07000', -23.4615, -46.5335, 'Guarulhos', 'SP'),
    ('08000', -23.5076, -46.4162, 'Sao Paulo ZL', 'SP'),
    ('09000', -23.6543, -46.5323, 'Santo Andre', 'SP'),

    -- Sao Paulo Interior
    ('13000', -22.9064, -47.0616, 'Campinas', 'SP'),
    ('14000', -21.1775, -47.8103, 'Ribeirao Preto', 'SP'),
    ('15000', -20.8113, -49.3760, 'Sao Jose do Rio Preto', 'SP'),
    ('16000', -21.2036, -50.4314, 'Aracatuba', 'SP'),
    ('17000', -22.3263, -49.0709, 'Bauru', 'SP'),
    ('18000', -23.5015, -47.4526, 'Sorocaba', 'SP'),
    ('19000', -22.1206, -51.3896, 'Presidente Prudente', 'SP'),

    -- Rio de Janeiro (20xxx-28xxx)
    ('20000', -22.9068, -43.1729, 'Rio de Janeiro Centro', 'RJ'),
    ('21000', -22.8569, -43.3507, 'Rio de Janeiro Norte', 'RJ'),
    ('22000', -22.9878, -43.2056, 'Rio de Janeiro Sul', 'RJ'),
    ('23000', -22.9136, -43.5675, 'Rio de Janeiro Oeste', 'RJ'),
    ('24000', -22.8833, -43.1036, 'Niteroi', 'RJ'),
    ('25000', -22.5105, -43.1820, 'Duque de Caxias', 'RJ'),
    ('26000', -22.7604, -43.4547, 'Nova Iguacu', 'RJ'),
    ('27000', -22.5005, -44.1001, 'Volta Redonda', 'RJ'),
    ('28000', -21.7625, -41.3306, 'Campos dos Goytacazes', 'RJ'),

    -- Minas Gerais (30xxx-39xxx)
    ('30000', -19.9191, -43.9386, 'Belo Horizonte', 'MG'),
    ('31000', -19.8575, -43.9190, 'Belo Horizonte Norte', 'MG'),
    ('32000', -20.0116, -44.0345, 'Contagem', 'MG'),
    ('33000', -19.8726, -43.8463, 'Santa Luzia', 'MG'),
    ('35000', -19.4667, -44.2490, 'Sete Lagoas', 'MG'),
    ('36000', -21.7621, -43.3490, 'Juiz de Fora', 'MG'),
    ('37000', -21.2492, -45.0021, 'Lavras', 'MG'),
    ('38000', -18.9186, -48.2772, 'Uberlandia', 'MG'),
    ('39000', -16.7325, -43.8682, 'Montes Claros', 'MG'),

    -- Parana (80xxx-87xxx)
    ('80000', -25.4284, -49.2733, 'Curitiba', 'PR'),
    ('81000', -25.4950, -49.3500, 'Curitiba Sul', 'PR'),
    ('82000', -25.3800, -49.2200, 'Curitiba Norte', 'PR'),
    ('83000', -25.5200, -49.1700, 'Sao Jose dos Pinhais', 'PR'),
    ('85000', -24.9573, -53.4551, 'Cascavel', 'PR'),
    ('86000', -23.3045, -51.1696, 'Londrina', 'PR'),
    ('87000', -23.4210, -51.9331, 'Maringa', 'PR'),

    -- Santa Catarina (88xxx-89xxx)
    ('88000', -27.5954, -48.5480, 'Florianopolis', 'SC'),
    ('89000', -26.3044, -48.8487, 'Joinville', 'SC'),

    -- Rio Grande do Sul (90xxx-99xxx)
    ('90000', -30.0346, -51.2177, 'Porto Alegre', 'RS'),
    ('91000', -30.0800, -51.1700, 'Porto Alegre Sul', 'RS'),
    ('92000', -29.9420, -51.0815, 'Canoas', 'RS'),
    ('93000', -29.7957, -51.1480, 'Sao Leopoldo', 'RS'),
    ('94000', -29.6880, -51.1307, 'Novo Hamburgo', 'RS'),
    ('95000', -29.1638, -51.1797, 'Caxias do Sul', 'RS'),
    ('96000', -31.7654, -52.3376, 'Pelotas', 'RS'),
    ('97000', -29.6842, -53.8069, 'Santa Maria', 'RS'),
    ('99000', -28.2621, -52.4066, 'Passo Fundo', 'RS'),

    -- Goias (74xxx-76xxx)
    ('74000', -16.6869, -49.2648, 'Goiania', 'GO'),
    ('75000', -16.3290, -48.9530, 'Anapolis', 'GO'),
    ('76000', -15.9436, -50.1412, 'Goias', 'GO'),

    -- Distrito Federal (70xxx-72xxx)
    ('70000', -15.7975, -47.8919, 'Brasilia', 'DF'),
    ('71000', -15.8594, -47.9594, 'Brasilia Sul', 'DF'),
    ('72000', -15.7800, -48.1300, 'Taguatinga', 'DF'),

    -- Bahia (40xxx-48xxx)
    ('40000', -12.9714, -38.5124, 'Salvador', 'BA'),
    ('41000', -12.9380, -38.4030, 'Salvador Norte', 'BA'),
    ('42000', -12.8960, -38.3570, 'Lauro de Freitas', 'BA'),
    ('44000', -12.2644, -38.9555, 'Feira de Santana', 'BA'),
    ('45000', -14.7900, -39.2776, 'Ilheus', 'BA'),

    -- Pernambuco (50xxx-56xxx)
    ('50000', -8.0476, -34.8770, 'Recife', 'PE'),
    ('51000', -8.1160, -34.8980, 'Recife Sul', 'PE'),
    ('53000', -7.9959, -34.8528, 'Olinda', 'PE'),
    ('55000', -8.2847, -35.9714, 'Caruaru', 'PE'),

    -- Ceara (60xxx-63xxx)
    ('60000', -3.7172, -38.5433, 'Fortaleza', 'CE'),
    ('61000', -3.7600, -38.6300, 'Fortaleza Sul', 'CE'),

    -- Para (66xxx-68xxx)
    ('66000', -1.4558, -48.5024, 'Belem', 'PA'),
    ('67000', -1.3680, -48.4880, 'Ananindeua', 'PA'),
    ('68000', -2.5010, -44.2826, 'Sao Luis', 'MA'),

    -- Amazonas (69xxx)
    ('69000', -3.1190, -60.0217, 'Manaus', 'AM'),

    -- Mato Grosso (78xxx)
    ('78000', -15.6014, -56.0979, 'Cuiaba', 'MT'),
    ('78700', -15.8919, -55.4967, 'Rondonopolis', 'MT')
ON CONFLICT (cep_prefix) DO NOTHING;
