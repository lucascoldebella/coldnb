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

export default function BannerFormModal({ banner, bannerType = "collection", onSave, onClose }) {
  const [form, setForm] = useState({
    title: "",
    subtitle: "",
    button_text: "Shop Now",
    button_link: "/shop-collection",
    image_url: "",
    image_alt: "",
    banner_type: bannerType,
    text_color: "dark",
    position: "left",
    countdown_end_at: "",
    discount_label: "",
    is_active: true,
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
    if (banner) {
      setForm({
        title: banner.title || "",
        subtitle: banner.subtitle || "",
        button_text: banner.button_text || "Shop Now",
        button_link: banner.button_link || "/shop-collection",
        image_url: banner.image_url || "",
        image_alt: banner.image_alt || "",
        banner_type: banner.banner_type || bannerType,
        text_color: banner.text_color || "dark",
        position: banner.position || "left",
        countdown_end_at: banner.countdown_end_at ? banner.countdown_end_at.slice(0, 16) : "",
        discount_label: banner.discount_label || "",
        is_active: banner.is_active !== false,
        sort_order: banner.sort_order || 0,
      });
    }
  }, [banner, bannerType]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      const data = { ...form };
      if (!data.countdown_end_at) data.countdown_end_at = null;
      data.sort_order = parseInt(data.sort_order) || 0;
      await onSave(data);
    } finally {
      setSaving(false);
    }
  };

  const update = (key, value) => setForm((prev) => ({ ...prev, [key]: value }));
  const isCountdown = form.banner_type === "countdown";

  return (
    <div className="admin-modal-overlay open">
      <div className="admin-modal md" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3 className="modal-title">{banner ? "Edit Banner" : `Add ${isCountdown ? "Countdown" : "Collection"} Banner`}</h3>
          <button type="button" onClick={onClose} className="modal-close"><XIcon /></button>
        </div>
        <form onSubmit={handleSubmit} style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}>
          <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>

            {/* Image */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}><ImageIcon /> Image</div>
              {form.image_url && (
                <div style={{ borderRadius: 8, overflow: "hidden", maxHeight: 120, marginBottom: 12, border: "1px solid var(--admin-border-light)" }}>
                  <img src={form.image_url} alt="Preview" style={{ width: "100%", objectFit: "cover", display: "block" }} />
                </div>
              )}
              <div className="form-group" style={{ marginBottom: 0 }}>
                <label>Image URL</label>
                <div style={{ display: "flex", gap: 8 }}>
                  <input className="admin-input" value={form.image_url} onChange={(e) => update("image_url", e.target.value)} style={{ flex: 1 }} />
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
            </div>

            {/* Content */}
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

            {/* Type-specific */}
            {!isCountdown && (
              <div style={fieldGroupStyle}>
                <div style={sectionLabelStyle}>Display Options</div>
                <div className="form-row cols-2">
                  <div className="form-group" style={{ marginBottom: 0 }}>
                    <label>Text Color</label>
                    <select className="admin-select" value={form.text_color} onChange={(e) => update("text_color", e.target.value)}>
                      <option value="dark">Dark</option>
                      <option value="white">White</option>
                    </select>
                  </div>
                  <div className="form-group" style={{ marginBottom: 0 }}>
                    <label>Position</label>
                    <select className="admin-select" value={form.position} onChange={(e) => update("position", e.target.value)}>
                      <option value="left">Left</option>
                      <option value="right">Right</option>
                    </select>
                  </div>
                </div>
              </div>
            )}
            {isCountdown && (
              <div style={fieldGroupStyle}>
                <div style={sectionLabelStyle}>Countdown</div>
                <div className="form-row cols-2">
                  <div className="form-group" style={{ marginBottom: 0 }}>
                    <label>Countdown End</label>
                    <input className="admin-input" type="datetime-local" value={form.countdown_end_at} onChange={(e) => update("countdown_end_at", e.target.value)} />
                  </div>
                  <div className="form-group" style={{ marginBottom: 0 }}>
                    <label>Discount Label</label>
                    <input className="admin-input" value={form.discount_label} onChange={(e) => update("discount_label", e.target.value)} placeholder="e.g. Up to 50% Off" />
                  </div>
                </div>
              </div>
            )}

            {/* Settings */}
            <div style={{ ...fieldGroupStyle, marginBottom: 0 }}>
              <div style={sectionLabelStyle}>Settings</div>
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
              {saving ? "Saving..." : banner ? "Update Banner" : "Create Banner"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
