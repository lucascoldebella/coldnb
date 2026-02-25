"use client";
import { useState, useEffect, useRef } from "react";

const XIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
  </svg>
);

const ImageIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <rect x="3" y="3" width="18" height="18" rx="2" ry="2" /><circle cx="8.5" cy="8.5" r="1.5" /><polyline points="21 15 16 10 5 21" />
  </svg>
);

const UploadIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" /><polyline points="17 8 12 3 7 8" /><line x1="12" y1="3" x2="12" y2="15" />
  </svg>
);

const fieldGroupStyle = {
  background: "var(--admin-bg)",
  borderRadius: "var(--admin-radius)",
  padding: "16px",
  marginBottom: 16,
  border: "1px solid var(--admin-border-light)",
};

const sectionLabelStyle = {
  fontSize: 11,
  fontWeight: 600,
  textTransform: "uppercase",
  letterSpacing: "0.05em",
  color: "var(--admin-primary)",
  marginBottom: 12,
  display: "flex",
  alignItems: "center",
  gap: 6,
};

export default function SlideFormModal({ slide, onSave, onClose }) {
  const [form, setForm] = useState({
    title: "",
    subtitle: "",
    button_text: "Explore Collection",
    button_link: "/shop-default-grid",
    image_url: "",
    image_alt: "fashion-slideshow",
    product_id: "",
    category_id: "",
    is_active: true,
    starts_at: "",
    ends_at: "",
    sort_order: 0,
  });
  const [saving, setSaving] = useState(false);
  const [uploading, setUploading] = useState(false);
  const fileInputRef = useRef(null);

  const handleFileUpload = async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    setUploading(true);
    try {
      const formData = new FormData();
      formData.append("files", file);
      const res = await fetch("/api/admin/upload", { method: "POST", body: formData });
      const json = await res.json();
      if (json.success && json.data?.urls?.[0]) {
        update("image_url", json.data.urls[0]);
      }
    } catch {
      // silently fail
    } finally {
      setUploading(false);
      if (fileInputRef.current) fileInputRef.current.value = "";
    }
  };

  useEffect(() => {
    if (slide) {
      setForm({
        title: slide.title || "",
        subtitle: slide.subtitle || "",
        button_text: slide.button_text || "Explore Collection",
        button_link: slide.button_link || "/shop-default-grid",
        image_url: slide.image_url || "",
        image_alt: slide.image_alt || "",
        product_id: slide.product_id || "",
        category_id: slide.category_id || "",
        is_active: slide.is_active !== false,
        starts_at: slide.starts_at ? slide.starts_at.slice(0, 16) : "",
        ends_at: slide.ends_at ? slide.ends_at.slice(0, 16) : "",
        sort_order: slide.sort_order || 0,
      });
    }
  }, [slide]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      const data = { ...form };
      if (data.product_id === "") data.product_id = null;
      else data.product_id = parseInt(data.product_id);
      if (data.category_id === "") data.category_id = null;
      else data.category_id = parseInt(data.category_id);
      if (!data.starts_at) data.starts_at = null;
      if (!data.ends_at) data.ends_at = null;
      data.sort_order = parseInt(data.sort_order) || 0;
      await onSave(data);
    } finally {
      setSaving(false);
    }
  };

  const update = (key, value) => setForm((prev) => ({ ...prev, [key]: value }));

  return (
    <div className="admin-modal-overlay open">
      <div className="admin-modal lg" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3 className="modal-title">{slide ? "Edit Slide" : "Add Hero Slide"}</h3>
          <button type="button" onClick={onClose} className="modal-close"><XIcon /></button>
        </div>
        <form onSubmit={handleSubmit} style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}>
          <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>

            {/* Image Section */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}><ImageIcon /> Image</div>
              {form.image_url && (
                <div style={{ borderRadius: 8, overflow: "hidden", maxHeight: 140, marginBottom: 12, border: "1px solid var(--admin-border-light)" }}>
                  <img src={form.image_url} alt="Preview" style={{ width: "100%", objectFit: "cover", display: "block" }} />
                </div>
              )}
              <div className="form-group" style={{ marginBottom: 12 }}>
                <label>Image URL</label>
                <div style={{ display: "flex", gap: 8 }}>
                  <input className="admin-input" value={form.image_url} onChange={(e) => update("image_url", e.target.value)} placeholder="/images/slider/..." style={{ flex: 1 }} />
                  <button
                    type="button"
                    className="admin-btn btn-secondary"
                    onClick={() => fileInputRef.current?.click()}
                    disabled={uploading}
                    style={{ whiteSpace: "nowrap", display: "flex", alignItems: "center", gap: 6 }}
                  >
                    <UploadIcon />
                    {uploading ? "Uploading..." : "Browse"}
                  </button>
                  <input ref={fileInputRef} type="file" accept="image/jpeg,image/png,image/gif,image/webp" onChange={handleFileUpload} style={{ display: "none" }} />
                </div>
              </div>
              <div className="form-group" style={{ marginBottom: 0 }}>
                <label>Alt Text</label>
                <input className="admin-input" value={form.image_alt} onChange={(e) => update("image_alt", e.target.value)} />
              </div>
            </div>

            {/* Content Section */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}>Content</div>
              <div className="form-row cols-2">
                <div className="form-group">
                  <label>Title</label>
                  <input className="admin-input" value={form.title} onChange={(e) => update("title", e.target.value)} />
                </div>
                <div className="form-group">
                  <label>Subtitle</label>
                  <input className="admin-input" value={form.subtitle} onChange={(e) => update("subtitle", e.target.value)} />
                </div>
              </div>
              <div className="form-row cols-2">
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Button Text</label>
                  <input className="admin-input" value={form.button_text} onChange={(e) => update("button_text", e.target.value)} />
                </div>
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Button Link</label>
                  <input className="admin-input" value={form.button_link} onChange={(e) => update("button_link", e.target.value)} />
                </div>
              </div>
            </div>

            {/* Linking Section */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}>Linking <span style={{ fontWeight: 400, textTransform: "none", letterSpacing: 0, color: "var(--admin-text-muted)" }}>— optional</span></div>
              <div className="form-row cols-2">
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Product ID</label>
                  <input className="admin-input" type="number" value={form.product_id} onChange={(e) => update("product_id", e.target.value)} placeholder="Auto-fills image" />
                </div>
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Category ID</label>
                  <input className="admin-input" type="number" value={form.category_id} onChange={(e) => update("category_id", e.target.value)} />
                </div>
              </div>
            </div>

            {/* Scheduling & Settings */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}>Schedule & Settings</div>
              <div className="form-row cols-2">
                <div className="form-group">
                  <label>Starts At</label>
                  <input className="admin-input" type="datetime-local" value={form.starts_at} onChange={(e) => update("starts_at", e.target.value)} />
                </div>
                <div className="form-group">
                  <label>Ends At</label>
                  <input className="admin-input" type="datetime-local" value={form.ends_at} onChange={(e) => update("ends_at", e.target.value)} />
                </div>
              </div>
              <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
                <div className="form-group" style={{ marginBottom: 0, width: 120 }}>
                  <label>Sort Order</label>
                  <input className="admin-input" type="number" value={form.sort_order} onChange={(e) => update("sort_order", e.target.value)} />
                </div>
                <label className="admin-toggle">
                  <span
                    className={`toggle-switch ${form.is_active ? "active" : ""}`}
                    onClick={() => update("is_active", !form.is_active)}
                  />
                  <span className="toggle-label">Active</span>
                </label>
              </div>
            </div>

          </div>
          <div className="modal-footer">
            <button type="button" onClick={onClose} className="admin-btn btn-secondary">Cancel</button>
            <button type="submit" className="admin-btn btn-primary" disabled={saving}>
              {saving ? "Saving..." : slide ? "Update Slide" : "Create Slide"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
