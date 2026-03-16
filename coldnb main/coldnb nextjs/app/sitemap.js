const BASE_URL = process.env.NEXT_PUBLIC_SITE_URL || "https://coldnb.com";
const API_URL = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8080";

export default async function sitemap() {
  const staticPages = [
    { url: `${BASE_URL}/`, changeFrequency: "daily", priority: 1.0 },
    { url: `${BASE_URL}/shop-default-grid`, changeFrequency: "daily", priority: 0.9 },
    { url: `${BASE_URL}/contact`, changeFrequency: "monthly", priority: 0.5 },
    { url: `${BASE_URL}/privacy-policy`, changeFrequency: "monthly", priority: 0.3 },
  ];

  let productPages = [];
  try {
    const res = await fetch(`${API_URL}/api/products?per_page=1000`, {
      cache: "no-store",
    });
    if (res.ok) {
      const data = await res.json();
      const products = data.data?.products || data.products || [];
      productPages = products.map((p) => ({
        url: `${BASE_URL}/product-detail/${p.id}`,
        lastModified: p.updated_at ? new Date(p.updated_at) : new Date(),
        changeFrequency: "weekly",
        priority: 0.8,
      }));
    }
  } catch {
    // Backend unavailable during build — return static pages only
  }

  return [...staticPages, ...productPages];
}
