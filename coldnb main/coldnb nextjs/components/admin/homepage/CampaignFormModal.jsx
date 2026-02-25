"use client";
import { useState, useEffect } from "react";

const XIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
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

export default function CampaignFormModal({ campaign, heroSlides = [], banners = [], onSave, onClose }) {
  const [form, setForm] = useState({
    name: "",
    description: "",
    starts_at: "",
    ends_at: "",
    is_active: false,
    hero_slide_ids: [],
    banner_ids: [],
  });
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    if (campaign) {
      setForm({
        name: campaign.name || "",
        description: campaign.description || "",
        starts_at: campaign.starts_at ? campaign.starts_at.slice(0, 16) : "",
        ends_at: campaign.ends_at ? campaign.ends_at.slice(0, 16) : "",
        is_active: campaign.is_active || false,
        hero_slide_ids: campaign.hero_slide_ids || [],
        banner_ids: campaign.banner_ids || [],
      });
    }
  }, [campaign]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!form.name.trim()) return;
    setSaving(true);
    try {
      const data = { ...form };
      if (!data.starts_at) data.starts_at = null;
      if (!data.ends_at) data.ends_at = null;
      await onSave(data);
    } finally {
      setSaving(false);
    }
  };

  const update = (key, value) => setForm((prev) => ({ ...prev, [key]: value }));

  const toggleArrayItem = (key, id) => {
    setForm((prev) => {
      const arr = prev[key] || [];
      return {
        ...prev,
        [key]: arr.includes(id) ? arr.filter((i) => i !== id) : [...arr, id],
      };
    });
  };

  return (
    <div className="admin-modal-overlay open">
      <div className="admin-modal md" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3 className="modal-title">{campaign ? "Edit Campaign" : "Create Campaign"}</h3>
          <button type="button" onClick={onClose} className="modal-close"><XIcon /></button>
        </div>
        <form onSubmit={handleSubmit} style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}>
          <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>

            {/* Details */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}>Details</div>
              <div className="form-group">
                <label>Campaign Name <span className="required">*</span></label>
                <input className="admin-input" value={form.name} onChange={(e) => update("name", e.target.value)} required />
              </div>
              <div className="form-group" style={{ marginBottom: 0 }}>
                <label>Description</label>
                <textarea className="admin-textarea" style={{ minHeight: 70 }} value={form.description} onChange={(e) => update("description", e.target.value)} />
              </div>
            </div>

            {/* Schedule */}
            <div style={fieldGroupStyle}>
              <div style={sectionLabelStyle}>Schedule</div>
              <div className="form-row cols-2">
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Starts At</label>
                  <input className="admin-input" type="datetime-local" value={form.starts_at} onChange={(e) => update("starts_at", e.target.value)} />
                </div>
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Ends At</label>
                  <input className="admin-input" type="datetime-local" value={form.ends_at} onChange={(e) => update("ends_at", e.target.value)} />
                </div>
              </div>
            </div>

            {/* Content Bundle */}
            {(heroSlides.length > 0 || banners.length > 0) && (
              <div style={fieldGroupStyle}>
                <div style={sectionLabelStyle}>Content Bundle</div>
                {heroSlides.length > 0 && (
                  <div className="form-group">
                    <label>Hero Slides</label>
                    <div style={{
                      display: "flex", flexDirection: "column", gap: 2,
                      maxHeight: 140, overflowY: "auto",
                      background: "var(--admin-surface)", borderRadius: "var(--admin-radius-sm)",
                      border: "1px solid var(--admin-border-light)", padding: 8,
                    }}>
                      {heroSlides.map((s) => (
                        <label key={s.id} className="admin-checkbox" style={{ padding: "6px 8px", borderRadius: 6 }}>
                          <input
                            type="checkbox"
                            checked={(form.hero_slide_ids || []).includes(s.id)}
                            onChange={() => toggleArrayItem("hero_slide_ids", s.id)}
                          />
                          <span className="checkbox-label">
                            {s.title || `Slide #${s.id}`}
                            {!s.is_active && <span style={{ color: "var(--admin-text-muted)", fontSize: 12, marginLeft: 6 }}>(inactive)</span>}
                          </span>
                        </label>
                      ))}
                    </div>
                  </div>
                )}
                {banners.length > 0 && (
                  <div className="form-group" style={{ marginBottom: 0 }}>
                    <label>Banners</label>
                    <div style={{
                      display: "flex", flexDirection: "column", gap: 2,
                      maxHeight: 140, overflowY: "auto",
                      background: "var(--admin-surface)", borderRadius: "var(--admin-radius-sm)",
                      border: "1px solid var(--admin-border-light)", padding: 8,
                    }}>
                      {banners.map((b) => (
                        <label key={b.id} className="admin-checkbox" style={{ padding: "6px 8px", borderRadius: 6 }}>
                          <input
                            type="checkbox"
                            checked={(form.banner_ids || []).includes(b.id)}
                            onChange={() => toggleArrayItem("banner_ids", b.id)}
                          />
                          <span className="checkbox-label">
                            {b.title || `Banner #${b.id}`}
                            <span style={{ color: "var(--admin-text-muted)", fontSize: 12, marginLeft: 4 }}>({b.banner_type})</span>
                            {!b.is_active && <span style={{ color: "var(--admin-text-muted)", fontSize: 12, marginLeft: 4 }}>(inactive)</span>}
                          </span>
                        </label>
                      ))}
                    </div>
                  </div>
                )}
              </div>
            )}

            {/* Active Toggle */}
            <div style={{ ...fieldGroupStyle, marginBottom: 0 }}>
              <label className="admin-toggle">
                <span
                  className={`toggle-switch ${form.is_active ? "active" : ""}`}
                  onClick={() => update("is_active", !form.is_active)}
                />
                <span className="toggle-label">Active</span>
              </label>
            </div>

          </div>
          <div className="modal-footer">
            <button type="button" onClick={onClose} className="admin-btn btn-secondary">Cancel</button>
            <button type="submit" className="admin-btn btn-primary" disabled={saving}>
              {saving ? "Saving..." : campaign ? "Update Campaign" : "Create Campaign"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
