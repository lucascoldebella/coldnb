"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminSections } from "@/lib/api/adminHomepage";
import ProductPicker from "./ProductPicker";

const SOURCE_TYPES = [
  { value: "manual", label: "Manual" },
  { value: "featured", label: "Featured" },
  { value: "new", label: "New Arrivals" },
  { value: "bestseller", label: "Best Sellers" },
  { value: "on_sale", label: "On Sale" },
  { value: "category", label: "By Category" },
];

export default function SectionsManager({ sections, onRefresh, hasPermission }) {
  const [editingId, setEditingId] = useState(null);
  const [editForm, setEditForm] = useState(null);
  const [managingProducts, setManagingProducts] = useState(null);
  const [products, setProducts] = useState([]);
  const [saving, setSaving] = useState(false);

  const startEditing = (section) => {
    setEditingId(section.id);
    setEditForm({
      title: section.title || "",
      subtitle: section.subtitle || "",
      source_type: section.source_type || "manual",
      category_id: section.category_id || "",
      max_items: section.max_items || 12,
      config: section.config || {},
      is_active: section.is_active !== false,
    });
  };

  const cancelEditing = () => {
    setEditingId(null);
    setEditForm(null);
  };

  const handleSaveSection = async () => {
    setSaving(true);
    try {
      const data = { ...editForm };
      if (data.category_id === "") data.category_id = null;
      else data.category_id = parseInt(data.category_id);
      data.max_items = parseInt(data.max_items) || 12;
      await adminSections.update(editingId, data);
      toast.success("Section updated");
      cancelEditing();
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update section");
    } finally {
      setSaving(false);
    }
  };

  const startManagingProducts = (section) => {
    setManagingProducts(section);
    const existing = (section.products || []).map((p, i) => ({
      product_id: p.product_id || p.id,
      product_name: p.product_name || p.name || `Product #${p.product_id || p.id}`,
      product_image: p.product_image || p.image_url || null,
      tab_name: p.tab_name || "",
      sort_order: p.sort_order ?? i,
    }));
    setProducts(existing);
  };

  const handleSaveProducts = async () => {
    setSaving(true);
    try {
      const mapped = products.map((p, i) => ({
        product_id: p.product_id,
        tab_name: p.tab_name || "",
        sort_order: i,
      }));
      await adminSections.updateProducts(managingProducts.id, mapped);
      toast.success("Products updated");
      setManagingProducts(null);
      setProducts([]);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update products");
    } finally {
      setSaving(false);
    }
  };

  const handleToggleActive = async (section) => {
    try {
      await adminSections.update(section.id, { is_active: !section.is_active });
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update section");
    }
  };

  const updateForm = (key, value) => setEditForm((prev) => ({ ...prev, [key]: value }));

  // Tab management for tabbed sections
  const getTabs = () => {
    if (!editForm?.config?.tabs) return [];
    return editForm.config.tabs;
  };

  const updateTab = (index, field, value) => {
    setEditForm((prev) => {
      const tabs = [...(prev.config?.tabs || [])];
      tabs[index] = { ...tabs[index], [field]: value };
      return { ...prev, config: { ...prev.config, tabs } };
    });
  };

  const addTab = () => {
    setEditForm((prev) => {
      const tabs = [...(prev.config?.tabs || [])];
      tabs.push({ name: "New Tab", source: "new" });
      return { ...prev, config: { ...prev.config, tabs } };
    });
  };

  const removeTab = (index) => {
    setEditForm((prev) => {
      const tabs = (prev.config?.tabs || []).filter((_, i) => i !== index);
      return { ...prev, config: { ...prev.config, tabs } };
    });
  };

  const getSectionLabel = (key) => {
    const labels = {
      products_tabbed: "Products (Tabbed)",
      shopgram: "ShopGram",
      testimonials: "Testimonials",
    };
    return labels[key] || key;
  };

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 16 }}>
        <h3 style={{ margin: 0, fontSize: 16 }}>Product Sections ({sections.length})</h3>
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
        {sections.map((section) => (
          <div key={section.id} className="admin-card" style={{ padding: 16 }}>
            {editingId === section.id ? (
              // Edit mode
              <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
                <div style={{ fontWeight: 600, fontSize: 14, marginBottom: 4 }}>
                  Editing: {getSectionLabel(section.section_key)}
                </div>
                <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
                  <div className="form-group">
                    <label className="admin-label">Title</label>
                    <input className="admin-input" value={editForm.title} onChange={(e) => updateForm("title", e.target.value)} />
                  </div>
                  <div className="form-group">
                    <label className="admin-label">Subtitle</label>
                    <input className="admin-input" value={editForm.subtitle} onChange={(e) => updateForm("subtitle", e.target.value)} />
                  </div>
                </div>
                <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 12 }}>
                  <div className="form-group">
                    <label className="admin-label">Source Type</label>
                    <select className="admin-input" value={editForm.source_type} onChange={(e) => updateForm("source_type", e.target.value)}>
                      {SOURCE_TYPES.map((s) => (
                        <option key={s.value} value={s.value}>{s.label}</option>
                      ))}
                    </select>
                  </div>
                  {editForm.source_type === "category" && (
                    <div className="form-group">
                      <label className="admin-label">Category ID</label>
                      <input className="admin-input" type="number" value={editForm.category_id} onChange={(e) => updateForm("category_id", e.target.value)} />
                    </div>
                  )}
                  <div className="form-group">
                    <label className="admin-label">Max Items</label>
                    <input className="admin-input" type="number" value={editForm.max_items} onChange={(e) => updateForm("max_items", e.target.value)} />
                  </div>
                </div>

                {section.section_key === "products_tabbed" && (
                  <div>
                    <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 8 }}>
                      <label className="admin-label" style={{ margin: 0 }}>Tabs</label>
                      <button className="admin-btn btn-sm btn-secondary" onClick={addTab}>+ Add Tab</button>
                    </div>
                    {getTabs().map((tab, i) => (
                      <div key={i} style={{ display: "flex", gap: 8, alignItems: "center", marginBottom: 6 }}>
                        <input
                          className="admin-input"
                          value={tab.name}
                          onChange={(e) => updateTab(i, "name", e.target.value)}
                          placeholder="Tab name"
                          style={{ flex: 1 }}
                        />
                        <select
                          className="admin-input"
                          value={tab.source || "new"}
                          onChange={(e) => updateTab(i, "source", e.target.value)}
                          style={{ flex: 1 }}
                        >
                          {SOURCE_TYPES.map((s) => (
                            <option key={s.value} value={s.value}>{s.label}</option>
                          ))}
                        </select>
                        <button
                          onClick={() => removeTab(i)}
                          style={{ border: "none", background: "none", cursor: "pointer", color: "#ef4444", fontSize: 16, padding: 4 }}
                        >&times;</button>
                      </div>
                    ))}
                  </div>
                )}

                <label style={{ display: "flex", alignItems: "center", gap: 8, cursor: "pointer" }}>
                  <input type="checkbox" checked={editForm.is_active} onChange={(e) => updateForm("is_active", e.target.checked)} />
                  Active
                </label>

                <div style={{ display: "flex", gap: 8, justifyContent: "flex-end" }}>
                  <button className="admin-btn btn-secondary btn-sm" onClick={cancelEditing}>Cancel</button>
                  <button className="admin-btn btn-primary btn-sm" onClick={handleSaveSection} disabled={saving}>
                    {saving ? "Saving..." : "Save"}
                  </button>
                </div>
              </div>
            ) : (
              // View mode
              <div style={{ display: "flex", gap: 12, alignItems: "center" }}>
                <div style={{ flex: 1 }}>
                  <div style={{ fontWeight: 500, fontSize: 14 }}>{section.title || getSectionLabel(section.section_key)}</div>
                  <div style={{ fontSize: 12, color: "#6b7280" }}>{section.subtitle}</div>
                  <div style={{ display: "flex", gap: 8, marginTop: 4, alignItems: "center" }}>
                    <span className={`admin-badge ${section.is_active ? "badge-success" : "badge-danger"}`}>
                      {section.is_active ? "Active" : "Inactive"}
                    </span>
                    <span style={{ fontSize: 11, color: "#9ca3af" }}>
                      Key: {section.section_key}
                    </span>
                    <span style={{ fontSize: 11, color: "#9ca3af" }}>
                      Source: {section.source_type}
                    </span>
                    <span style={{ fontSize: 11, color: "#9ca3af" }}>
                      Max: {section.max_items}
                    </span>
                    {section.products?.length > 0 && (
                      <span style={{ fontSize: 11, color: "#9ca3af" }}>
                        Products: {section.products.length}
                      </span>
                    )}
                  </div>
                </div>
                {hasPermission && (
                  <div style={{ display: "flex", gap: 4, flexShrink: 0 }}>
                    <button className="admin-btn btn-sm btn-secondary" onClick={() => handleToggleActive(section)}>
                      {section.is_active ? "Deactivate" : "Activate"}
                    </button>
                    <button className="admin-btn btn-sm btn-secondary" onClick={() => startEditing(section)}>Edit</button>
                    {section.source_type === "manual" && (
                      <button className="admin-btn btn-sm btn-primary" onClick={() => startManagingProducts(section)}>
                        Products
                      </button>
                    )}
                  </div>
                )}
              </div>
            )}
          </div>
        ))}
        {sections.length === 0 && (
          <div style={{ textAlign: "center", padding: 32, color: "#9ca3af" }}>
            No sections configured. Run the database migration to seed defaults.
          </div>
        )}
      </div>

      {/* Product picker modal */}
      {managingProducts && (
        <div className="admin-modal-overlay open" onClick={() => { setManagingProducts(null); setProducts([]); }}>
          <div className="admin-modal md" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3 className="modal-title">Manage Products — {managingProducts.title || getSectionLabel(managingProducts.section_key)}</h3>
              <button onClick={() => { setManagingProducts(null); setProducts([]); }} className="modal-close">&times;</button>
            </div>
            <div className="modal-body">
              <ProductPicker
                selected={products}
                onChange={setProducts}
                max={managingProducts.max_items || 20}
              />
            </div>
            <div className="modal-footer">
              <button className="admin-btn btn-secondary" onClick={() => { setManagingProducts(null); setProducts([]); }}>Cancel</button>
              <button className="admin-btn btn-primary" onClick={handleSaveProducts} disabled={saving}>
                {saving ? "Saving..." : "Save Products"}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
