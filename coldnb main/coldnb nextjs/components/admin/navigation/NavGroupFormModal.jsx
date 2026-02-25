"use client";
import { useState, useEffect } from "react";

const XIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
  </svg>
);

export default function NavGroupFormModal({ group, onSave, onClose }) {
  const [form, setForm] = useState({
    title: "",
    translation_key: "",
    sort_order: 0,
    is_active: true,
  });
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    if (group) {
      setForm({
        title: group.title || "",
        translation_key: group.translation_key || "",
        sort_order: group.sort_order || 0,
        is_active: group.is_active !== false,
      });
    }
  }, [group]);

  const update = (key, val) => setForm((prev) => ({ ...prev, [key]: val }));

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
      <div className="admin-modal sm" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3 className="modal-title">{group ? "Edit Group" : "Add Group"}</h3>
          <button className="modal-close" onClick={onClose}><XIcon /></button>
        </div>
        <form onSubmit={handleSubmit} style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}>
          <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>
            <div className="form-group">
              <label>Title</label>
              <input className="admin-input" value={form.title} onChange={(e) => update("title", e.target.value)} placeholder="Column heading (optional)" />
            </div>
            <div className="form-group">
              <label>Translation Key</label>
              <input className="admin-input" value={form.translation_key} onChange={(e) => update("translation_key", e.target.value)} placeholder="e.g. nav.shopLayout" />
            </div>
            <div className="form-group">
              <label>Sort Order</label>
              <input className="admin-input" type="number" value={form.sort_order} onChange={(e) => update("sort_order", parseInt(e.target.value) || 0)} />
            </div>
            <div className="admin-toggle" onClick={() => update("is_active", !form.is_active)}>
              <span className={`toggle-switch ${form.is_active ? "active" : ""}`} />
              <span className="toggle-label">Active</span>
            </div>
          </div>
          <div className="modal-footer">
            <button type="button" className="admin-btn btn-secondary" onClick={onClose}>Cancel</button>
            <button type="submit" className="admin-btn btn-primary" disabled={saving}>
              {saving ? "Saving..." : group ? "Update" : "Create"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
