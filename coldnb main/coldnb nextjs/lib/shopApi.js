import axios from "axios";
import { getApiBaseUrl } from "./apiBase";

const shopApi = axios.create({
  baseURL: getApiBaseUrl(),
  headers: {
    "Content-Type": "application/json",
  },
  timeout: 15000,
});

/**
 * Transform a backend product object into the frontend product shape.
 * The backend returns: { id, name, slug, price, compare_at_price, images: [{url, is_primary}], ... }
 * The frontend expects: { id, title, price, imgSrc, imgHover, oldPrice, ... }
 */
export function transformProduct(product) {
  const primaryImage = product.images?.find((img) => img.is_primary) || product.images?.[0];
  const hoverImage = product.images?.[1] || primaryImage;

  return {
    id: product.id,
    title: product.name,
    name: product.name,
    price: parseFloat(product.price) || 0,
    oldPrice: product.compare_at_price ? parseFloat(product.compare_at_price) : null,
    compareAtPrice: product.compare_at_price ? parseFloat(product.compare_at_price) : null,
    description: product.description || "",
    shortDescription: product.short_description || "",
    sku: product.sku,
    slug: product.slug,
    category: product.category_name || "Uncategorized",
    category_id: product.category_id,
    brand: product.brand || "",
    stock: product.stock_quantity || 0,
    isActive: product.is_active,
    isFeatured: product.is_featured,
    isNew: product.is_new,
    isSale: product.is_sale,
    isOnSale: product.is_sale,
    imgSrc: primaryImage?.url || "/images/products/placeholder.jpg",
    imgHover: hoverImage?.url || primaryImage?.url || "/images/products/placeholder.jpg",
    images: product.images?.map((img, index) => ({
      id: img.id || index + 1,
      url: img.url,
      src: img.url,
      is_primary: img.is_primary,
      alt: product.name || "",
    })) || [],
    colors: product.colors?.map((c) => ({
      name: c.name,
      code: c.hex_code,
      bgColor: `bg-${c.name.toLowerCase().replace(/\s+/g, "-")}`,
      imgSrc: c.image_url || primaryImage?.url || "/images/products/placeholder.jpg",
    })) || [],
    sizes: product.sizes?.map((s) => s.name) || [],
    inStock: (product.stock_quantity || 0) > 0,
  };
}

/**
 * Get paginated products list with optional filters
 * @param {Object} params - { category, min_price, max_price, sort, page, per_page, featured, sale }
 */
export async function getProducts(params = {}) {
  const response = await shopApi.get("/api/products", { params });
  const data = response.data;
  return {
    products: (data.data || []).map(transformProduct),
    pagination: data.pagination || { page: 1, per_page: 20, total: 0, total_pages: 0 },
  };
}

/**
 * Search products by query string
 * @param {string} query - Search term
 */
export async function searchProducts(query) {
  const response = await shopApi.get("/api/products/search", { params: { q: query } });
  return (response.data.data || []).map(transformProduct);
}

/**
 * Get categories list
 */
export async function getCategories() {
  const response = await shopApi.get("/api/categories");
  return response.data.data || [];
}

/**
 * Get a single product by ID or slug
 * @param {string|number} id - Product ID or slug
 */
export async function getProduct(id) {
  const response = await shopApi.get(`/api/products/${id}`);
  if (response.data.success && response.data.data) {
    return transformProduct(response.data.data);
  }
  return null;
}

/**
 * Calculate shipping cost based on Brazilian CEP
 * @param {string} cep - CEP in format XXXXX-XXX or XXXXXXXX
 */
export async function calculateShipping(cep) {
  const cleanCep = cep.replace(/\D/g, "");
  const response = await shopApi.get("/api/shipping/calculate", { params: { cep: cleanCep } });
  const result = response.data.data || response.data;
  if (!result || !result.price) {
    throw new Error(result?.message || "Could not calculate shipping for this CEP");
  }
  return result;
}

/**
 * Get multiple products by their IDs (parallel fetch)
 * @param {Array<number>} ids - Array of product IDs
 * @returns {Array} Array of transformed products (skips failed fetches)
 */
export async function getProductsByIds(ids) {
  if (!ids || ids.length === 0) return [];
  const results = await Promise.all(
    ids.map((id) => getProduct(id).catch(() => null))
  );
  return results.filter(Boolean);
}

export default shopApi;
