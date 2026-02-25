"use client";
import { useState, useEffect, useCallback } from "react";
import { adminProducts } from "@/lib/api/adminProducts";

export default function ProductPicker({ selected = [], onChange, max = 20 }) {
  const [search, setSearch] = useState("");
  const [results, setResults] = useState([]);
  const [loading, setLoading] = useState(false);

  const searchProducts = useCallback(async (query) => {
    if (!query || query.length < 2) {
      setResults([]);
      return;
    }
    setLoading(true);
    try {
      const res = await adminProducts.list({ search: query, limit: 20 });
      const data = res.data?.data?.products || res.data?.products || res.data?.data || [];
      setResults(Array.isArray(data) ? data : []);
    } catch {
      setResults([]);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    const timer = setTimeout(() => searchProducts(search), 300);
    return () => clearTimeout(timer);
  }, [search, searchProducts]);

  const addProduct = (product) => {
    if (selected.length >= max) return;
    if (selected.find((p) => p.product_id === product.id)) return;
    onChange([...selected, {
      product_id: product.id,
      product_name: product.name,
      product_image: product.image_url || null,
      sort_order: selected.length,
    }]);
    setSearch("");
    setResults([]);
  };

  const removeProduct = (productId) => {
    onChange(selected.filter((p) => p.product_id !== productId));
  };

  const moveUp = (index) => {
    if (index === 0) return;
    const items = [...selected];
    [items[index - 1], items[index]] = [items[index], items[index - 1]];
    onChange(items.map((p, i) => ({ ...p, sort_order: i })));
  };

  const moveDown = (index) => {
    if (index === selected.length - 1) return;
    const items = [...selected];
    [items[index], items[index + 1]] = [items[index + 1], items[index]];
    onChange(items.map((p, i) => ({ ...p, sort_order: i })));
  };

  return (
    <div className="product-picker">
      <div style={{ marginBottom: 12 }}>
        <input
          type="text"
          className="admin-input"
          placeholder="Search products to add..."
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          style={{ width: "100%" }}
        />
        {loading && <div style={{ fontSize: 12, color: "#6b7280", marginTop: 4 }}>Searching...</div>}
        {results.length > 0 && (
          <div style={{
            border: "1px solid var(--admin-border)",
            borderRadius: 8,
            maxHeight: 200,
            overflow: "auto",
            marginTop: 4,
            background: "var(--admin-card-bg)",
          }}>
            {results
              .filter((r) => !selected.find((s) => s.product_id === r.id))
              .map((product) => (
                <div
                  key={product.id}
                  onClick={() => addProduct(product)}
                  style={{
                    display: "flex",
                    alignItems: "center",
                    gap: 10,
                    padding: "8px 12px",
                    cursor: "pointer",
                    borderBottom: "1px solid var(--admin-border)",
                  }}
                  onMouseEnter={(e) => (e.currentTarget.style.background = "var(--admin-hover)")}
                  onMouseLeave={(e) => (e.currentTarget.style.background = "transparent")}
                >
                  <div style={{
                    width: 32, height: 32, borderRadius: 6, overflow: "hidden",
                    backgroundColor: "#f3f4f6", flexShrink: 0,
                  }}>
                    {product.image_url && <img src={product.image_url} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />}
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ fontSize: 13, fontWeight: 500 }}>{product.name}</div>
                    <div style={{ fontSize: 11, color: "#6b7280" }}>ID: {product.id}</div>
                  </div>
                  <button className="admin-btn btn-sm btn-primary" style={{ padding: "2px 8px", fontSize: 11 }}>Add</button>
                </div>
              ))}
          </div>
        )}
      </div>

      {selected.length > 0 && (
        <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
          {selected.map((item, index) => (
            <div
              key={item.product_id}
              style={{
                display: "flex",
                alignItems: "center",
                gap: 10,
                padding: "6px 10px",
                border: "1px solid var(--admin-border)",
                borderRadius: 6,
                background: "var(--admin-card-bg)",
              }}
            >
              <span style={{ fontSize: 12, color: "#9ca3af", width: 20 }}>{index + 1}</span>
              <div style={{
                width: 28, height: 28, borderRadius: 4, overflow: "hidden",
                backgroundColor: "#f3f4f6", flexShrink: 0,
              }}>
                {item.product_image && <img src={item.product_image} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />}
              </div>
              <div style={{ flex: 1, fontSize: 13 }}>{item.product_name}</div>
              <div style={{ display: "flex", gap: 2 }}>
                <button
                  onClick={() => moveUp(index)}
                  disabled={index === 0}
                  style={{ border: "none", background: "none", cursor: "pointer", padding: 2, color: index === 0 ? "#d1d5db" : "#6b7280", fontSize: 14 }}
                  title="Move up"
                >&#9650;</button>
                <button
                  onClick={() => moveDown(index)}
                  disabled={index === selected.length - 1}
                  style={{ border: "none", background: "none", cursor: "pointer", padding: 2, color: index === selected.length - 1 ? "#d1d5db" : "#6b7280", fontSize: 14 }}
                  title="Move down"
                >&#9660;</button>
                <button
                  onClick={() => removeProduct(item.product_id)}
                  style={{ border: "none", background: "none", cursor: "pointer", padding: 2, color: "#ef4444", fontSize: 14 }}
                  title="Remove"
                >&times;</button>
              </div>
            </div>
          ))}
        </div>
      )}

      {selected.length === 0 && (
        <div style={{ fontSize: 13, color: "#9ca3af", textAlign: "center", padding: 16 }}>
          No products selected. Search above to add products.
        </div>
      )}
    </div>
  );
}
