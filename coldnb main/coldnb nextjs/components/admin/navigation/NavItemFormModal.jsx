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

export default function NavItemFormModal({ item, showImageFields, onSave, onClose }) {
  const [form, setForm] = useState({
    label: "",
    href: "",
    image_url: "",
    image_alt: "",
    badge: "",
    sort_order: 0,
    is_active: true,
  });
  const [saving, setSaving] = useState(false);
  const [uploading, setUploading] = useState(false);
  const fileInputRef = useRef(null);

  useEffect(() => {
    if (item) {
      setForm({
        label: item.label || "",
        href: item.href || "",
        image_url: item.image_url || "",
        image_alt: item.image_alt || "",
        badge: item.badge || "",
        sort_order: item.sort_order || 0,
        is_active: item.is_active !== false,
      });
    }
  }, [item]);

  const update = (key, val) => setForm((prev) => ({ ...prev, [key]: val }));

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

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    try {
      await onSave(form);
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="admin-modal-overlay open" onClick={onClose}>
      <div className="admin-modal md" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3 className="modal-title">{item ? "Edit Item" : "Add Item"}</h3>
          <button className="modal-close" onClick={onClose}><XIcon /></button>
        </div>
        <form onSubmit={handleSubmit} style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}>
          <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>
            <div className="form-row cols-2">
              <div className="form-group">
                <label>Label *</label>
                <input className="admin-input" value={form.label} onChange={(e) => update("label", e.target.value)} required />
              </div>
              <div className="form-group">
                <label>Link (href) *</label>
                <input className="admin-input" value={form.href} onChange={(e) => update("href", e.target.value)} required />
              </div>
            </div>

            {showImageFields && (
              <>
                <div className="form-group">
                  <label>Image URL</label>
                  <div style={{ display: "flex", gap: 8 }}>
                    <input className="admin-input" value={form.image_url} onChange={(e) => update("image_url", e.target.value)} style={{ flex: 1 }} />
                    <button type="button" className="admin-btn btn-secondary btn-sm" onClick={() => fileInputRef.current?.click()} disabled={uploading}>
                      {uploading ? "..." : <><UploadIcon /> Browse</>}
                    </button>
                    <input ref={fileInputRef} type="file" accept="image/*" style={{ display: "none" }} onChange={handleFileUpload} />
                  </div>
                  {form.image_url && (
                    <div style={{ marginTop: 8, borderRadius: 6, overflow: "hidden", width: 120, height: 80, background: "#f3f4f6" }}>
                      <img src={form.image_url} alt={form.image_alt} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                    </div>
                  )}
                </div>
                <div className="form-group">
                  <label>Image Alt Text</label>
                  <input className="admin-input" value={form.image_alt} onChange={(e) => update("image_alt", e.target.value)} />
                </div>
              </>
            )}

            <div className="form-row cols-2">
              <div className="form-group">
                <label>Badge</label>
                <input className="admin-input" value={form.badge} onChange={(e) => update("badge", e.target.value)} placeholder="e.g. New, Hot" />
              </div>
              <div className="form-group">
                <label>Sort Order</label>
                <input className="admin-input" type="number" value={form.sort_order} onChange={(e) => update("sort_order", parseInt(e.target.value) || 0)} />
              </div>
            </div>

            <div className="admin-toggle" onClick={() => update("is_active", !form.is_active)}>
              <span className={`toggle-switch ${form.is_active ? "active" : ""}`} />
              <span className="toggle-label">Active</span>
            </div>
          </div>
          <div className="modal-footer">
            <button type="button" className="admin-btn btn-secondary" onClick={onClose}>Cancel</button>
            <button type="submit" className="admin-btn btn-primary" disabled={saving}>
              {saving ? "Saving..." : item ? "Update" : "Create"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
