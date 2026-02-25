"use client";
import { useState, useEffect } from "react";
import Link from "next/link";
import { adminCategories } from "@/lib/api/adminProducts";

export default function CategoriesManager() {
  const [categories, setCategories] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchCategories = async () => {
      try {
        const res = await adminCategories.list();
        const data = res.data?.data?.categories || res.data?.categories || res.data?.data || [];
        setCategories(Array.isArray(data) ? data : []);
      } catch {
        setCategories([]);
      } finally {
        setLoading(false);
      }
    };
    fetchCategories();
  }, []);

  if (loading) {
    return <div style={{ color: "#9ca3af", fontSize: 13, padding: 8 }}>Loading categories...</div>;
  }

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)" }}>
          Category carousel shown below the hero slider. Manage categories in the Categories page.
        </div>
        <Link href="/admin/categories" className="admin-btn btn-primary btn-sm">
          Manage Categories
        </Link>
      </div>

      {categories.length > 0 ? (
        <div style={{ display: "flex", gap: 12, overflowX: "auto", paddingBottom: 4 }}>
          {categories.slice(0, 8).map((cat) => (
            <div key={cat.id} style={{
              minWidth: 90, textAlign: "center", flexShrink: 0,
            }}>
              <div style={{
                width: 72, height: 72, borderRadius: "50%", overflow: "hidden",
                backgroundColor: "#f3f4f6", margin: "0 auto 6px",
              }}>
                {cat.image_url && (
                  <img src={cat.image_url} alt={cat.name} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                )}
              </div>
              <div style={{ fontSize: 12, fontWeight: 500 }}>{cat.name}</div>
              {cat.product_count != null && (
                <div style={{ fontSize: 11, color: "#9ca3af" }}>{cat.product_count} items</div>
              )}
            </div>
          ))}
        </div>
      ) : (
        <div style={{ textAlign: "center", padding: 20, color: "#9ca3af", fontSize: 13 }}>
          No categories found. Create categories in the Categories page.
        </div>
      )}
    </div>
  );
}
