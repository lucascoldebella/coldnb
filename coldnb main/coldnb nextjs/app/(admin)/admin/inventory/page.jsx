"use client";
import { useState, useEffect, useCallback } from "react";
import adminApi from "@/lib/adminApi";
import toast from "react-hot-toast";
import { formatCurrency } from "@/lib/adminUtils";

const StockBadge = ({ quantity }) => {
  let color, bg, label;
  if (quantity <= 0) {
    color = "#dc2626"; bg = "#fef2f2"; label = "Out of Stock";
  } else if (quantity <= 10) {
    color = "#d97706"; bg = "#fffbeb"; label = "Low Stock";
  } else if (quantity <= 20) {
    color = "#ca8a04"; bg = "#fefce8"; label = "Medium";
  } else {
    color = "#16a34a"; bg = "#f0fdf4"; label = "In Stock";
  }
  return (
    <span style={{
      display: "inline-flex", alignItems: "center", gap: 6,
      padding: "4px 10px", borderRadius: 6, fontSize: 12, fontWeight: 600,
      color, backgroundColor: bg,
    }}>
      <span style={{ width: 6, height: 6, borderRadius: "50%", backgroundColor: color }} />
      {label} ({quantity})
    </span>
  );
};

export default function InventoryPage() {
  const [products, setProducts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [filter, setFilter] = useState("all"); // all, low, out, in
  const [search, setSearch] = useState("");
  const [editingId, setEditingId] = useState(null);
  const [editValue, setEditValue] = useState("");
  const [saving, setSaving] = useState(false);
  const [categories, setCategories] = useState([]);
  const [categoryFilter, setCategoryFilter] = useState("");
  const [sortBy, setSortBy] = useState("stock_asc");

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const [prodRes, catRes] = await Promise.all([
        adminApi.get("/api/admin/products", { params: { limit: 500 } }),
        adminApi.get("/api/admin/categories"),
      ]);
      const prodData = prodRes.data?.data?.products || prodRes.data?.products || [];
      const catData = catRes.data?.data?.categories || catRes.data?.categories || [];
      setProducts(prodData);
      setCategories(catData);
    } catch {
      toast.error("Failed to load inventory data");
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => { fetchData(); }, [fetchData]);

  const handleStockUpdate = async (productId) => {
    const newStock = parseInt(editValue, 10);
    if (isNaN(newStock) || newStock < 0) {
      toast.error("Invalid stock quantity");
      return;
    }
    setSaving(true);
    try {
      await adminApi.put(`/api/admin/products/${productId}`, { stock_quantity: newStock });
      setProducts(prev => prev.map(p => p.id === productId ? { ...p, stock_quantity: newStock } : p));
      setEditingId(null);
      toast.success("Stock updated");
    } catch {
      toast.error("Failed to update stock");
    } finally {
      setSaving(false);
    }
  };

  // Filter + search + sort
  let filtered = products.filter(p => {
    if (filter === "out" && p.stock_quantity > 0) return false;
    if (filter === "low" && (p.stock_quantity <= 0 || p.stock_quantity > 20)) return false;
    if (filter === "in" && p.stock_quantity <= 20) return false;
    if (categoryFilter && String(p.category_id) !== categoryFilter) return false;
    if (search) {
      const s = search.toLowerCase();
      return (p.name || "").toLowerCase().includes(s) ||
             (p.sku || "").toLowerCase().includes(s);
    }
    return true;
  });

  filtered.sort((a, b) => {
    switch (sortBy) {
      case "stock_asc": return (a.stock_quantity || 0) - (b.stock_quantity || 0);
      case "stock_desc": return (b.stock_quantity || 0) - (a.stock_quantity || 0);
      case "name_asc": return (a.name || "").localeCompare(b.name || "");
      case "price_desc": return (parseFloat(b.price) || 0) - (parseFloat(a.price) || 0);
      default: return 0;
    }
  });

  // Stats
  const totalProducts = products.length;
  const outOfStock = products.filter(p => p.stock_quantity <= 0).length;
  const lowStock = products.filter(p => p.stock_quantity > 0 && p.stock_quantity <= 10).length;
  const totalUnits = products.reduce((sum, p) => sum + (p.stock_quantity || 0), 0);
  const totalValue = products.reduce((sum, p) => sum + (parseFloat(p.price) || 0) * (p.stock_quantity || 0), 0);

  if (loading) {
    return (
      <div className="inventory-page">
        <div className="admin-page-header">
          <h1 className="page-title">Inventory</h1>
        </div>
        <div className="admin-card">
          <div className="card-body" style={{ padding: 40, textAlign: "center" }}>
            <div className="spinner-border" role="status" />
            <p style={{ marginTop: 12, color: "#6b7280" }}>Loading inventory...</p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="inventory-page">
      <div className="admin-page-header">
        <h1 className="page-title">Inventory Management</h1>
        <div className="page-actions">
          <button className="admin-btn btn-secondary" onClick={fetchData}>
            Refresh
          </button>
        </div>
      </div>

      {/* Stats Cards */}
      <div style={{ display: "grid", gridTemplateColumns: "repeat(5, 1fr)", gap: 16, marginBottom: 24 }}>
        <div className="admin-card" style={{ cursor: "pointer" }} onClick={() => setFilter("all")}>
          <div className="card-body" style={{ padding: 16, textAlign: "center" }}>
            <div style={{ fontSize: 28, fontWeight: 700 }}>{totalProducts}</div>
            <div style={{ fontSize: 13, color: "#6b7280" }}>Total Products</div>
          </div>
        </div>
        <div className="admin-card" style={{ cursor: "pointer", borderLeft: outOfStock > 0 ? "3px solid #dc2626" : undefined }} onClick={() => setFilter("out")}>
          <div className="card-body" style={{ padding: 16, textAlign: "center" }}>
            <div style={{ fontSize: 28, fontWeight: 700, color: outOfStock > 0 ? "#dc2626" : undefined }}>{outOfStock}</div>
            <div style={{ fontSize: 13, color: "#6b7280" }}>Out of Stock</div>
          </div>
        </div>
        <div className="admin-card" style={{ cursor: "pointer", borderLeft: lowStock > 0 ? "3px solid #d97706" : undefined }} onClick={() => setFilter("low")}>
          <div className="card-body" style={{ padding: 16, textAlign: "center" }}>
            <div style={{ fontSize: 28, fontWeight: 700, color: lowStock > 0 ? "#d97706" : undefined }}>{lowStock}</div>
            <div style={{ fontSize: 13, color: "#6b7280" }}>Low Stock (&le;10)</div>
          </div>
        </div>
        <div className="admin-card">
          <div className="card-body" style={{ padding: 16, textAlign: "center" }}>
            <div style={{ fontSize: 28, fontWeight: 700 }}>{totalUnits.toLocaleString()}</div>
            <div style={{ fontSize: 13, color: "#6b7280" }}>Total Units</div>
          </div>
        </div>
        <div className="admin-card">
          <div className="card-body" style={{ padding: 16, textAlign: "center" }}>
            <div style={{ fontSize: 28, fontWeight: 700 }}>{formatCurrency(totalValue)}</div>
            <div style={{ fontSize: 13, color: "#6b7280" }}>Inventory Value</div>
          </div>
        </div>
      </div>

      {/* Filters */}
      <div className="admin-card" style={{ marginBottom: 24 }}>
        <div className="card-body" style={{ padding: 16 }}>
          <div style={{ display: "flex", gap: 12, flexWrap: "wrap", alignItems: "center" }}>
            <input
              type="text"
              className="form-control"
              placeholder="Search by name or SKU..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              style={{ maxWidth: 280 }}
            />
            <select
              className="admin-select"
              value={filter}
              onChange={(e) => setFilter(e.target.value)}
              style={{ maxWidth: 160 }}
            >
              <option value="all">All Stock Levels</option>
              <option value="out">Out of Stock</option>
              <option value="low">Low Stock (&le;20)</option>
              <option value="in">In Stock (&gt;20)</option>
            </select>
            <select
              className="admin-select"
              value={categoryFilter}
              onChange={(e) => setCategoryFilter(e.target.value)}
              style={{ maxWidth: 180 }}
            >
              <option value="">All Categories</option>
              {categories.map(c => (
                <option key={c.id} value={String(c.id)}>{c.name}</option>
              ))}
            </select>
            <select
              className="admin-select"
              value={sortBy}
              onChange={(e) => setSortBy(e.target.value)}
              style={{ maxWidth: 180 }}
            >
              <option value="stock_asc">Stock: Low to High</option>
              <option value="stock_desc">Stock: High to Low</option>
              <option value="name_asc">Name: A-Z</option>
              <option value="price_desc">Price: High to Low</option>
            </select>
            <span style={{ fontSize: 13, color: "#6b7280", marginLeft: "auto" }}>
              Showing {filtered.length} of {totalProducts}
            </span>
          </div>
        </div>
      </div>

      {/* Products Table */}
      <div className="admin-card">
        <div className="card-body" style={{ padding: 0 }}>
          <table className="admin-table">
            <thead>
              <tr>
                <th>Product</th>
                <th style={{ width: 100 }}>SKU</th>
                <th style={{ width: 100, textAlign: "right" }}>Price</th>
                <th style={{ width: 140, textAlign: "center" }}>Stock Level</th>
                <th style={{ width: 180, textAlign: "center" }}>Update Stock</th>
              </tr>
            </thead>
            <tbody>
              {filtered.length === 0 ? (
                <tr>
                  <td colSpan={5} style={{ textAlign: "center", padding: 40, color: "#9ca3af" }}>
                    No products match your filters.
                  </td>
                </tr>
              ) : (
                filtered.map(product => (
                  <tr key={product.id}>
                    <td>
                      <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
                        <div style={{ width: 40, height: 40, borderRadius: 8, backgroundColor: "#f3f4f6", overflow: "hidden", flexShrink: 0 }}>
                          {product.primary_image && (
                            <img src={product.primary_image} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                          )}
                        </div>
                        <div>
                          <div style={{ fontWeight: 500, fontSize: 14 }}>{product.name}</div>
                          {!product.is_active && (
                            <span style={{ fontSize: 11, color: "#9ca3af", fontStyle: "italic" }}>Inactive</span>
                          )}
                        </div>
                      </div>
                    </td>
                    <td style={{ fontSize: 13, fontFamily: "monospace", color: "#6b7280" }}>
                      {product.sku || "-"}
                    </td>
                    <td style={{ textAlign: "right", fontWeight: 500 }}>
                      {formatCurrency(product.price)}
                    </td>
                    <td style={{ textAlign: "center" }}>
                      <StockBadge quantity={product.stock_quantity || 0} />
                    </td>
                    <td style={{ textAlign: "center" }}>
                      {editingId === product.id ? (
                        <div style={{ display: "flex", gap: 4, justifyContent: "center" }}>
                          <input
                            type="number"
                            min="0"
                            className="form-control"
                            value={editValue}
                            onChange={(e) => setEditValue(e.target.value)}
                            onKeyDown={(e) => {
                              if (e.key === "Enter") handleStockUpdate(product.id);
                              if (e.key === "Escape") setEditingId(null);
                            }}
                            style={{ width: 70, textAlign: "center", padding: "4px 8px" }}
                            autoFocus
                          />
                          <button
                            className="admin-btn btn-primary btn-sm"
                            onClick={() => handleStockUpdate(product.id)}
                            disabled={saving}
                            style={{ padding: "4px 10px", fontSize: 12 }}
                          >
                            {saving ? "..." : "Save"}
                          </button>
                          <button
                            className="admin-btn btn-ghost btn-sm"
                            onClick={() => setEditingId(null)}
                            style={{ padding: "4px 8px", fontSize: 12 }}
                          >
                            X
                          </button>
                        </div>
                      ) : (
                        <button
                          className="admin-btn btn-secondary btn-sm"
                          onClick={() => {
                            setEditingId(product.id);
                            setEditValue(String(product.stock_quantity || 0));
                          }}
                          style={{ fontSize: 12 }}
                        >
                          Edit Stock
                        </button>
                      )}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
