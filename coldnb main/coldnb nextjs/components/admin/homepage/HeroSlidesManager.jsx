"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminHeroSlides } from "@/lib/api/adminHomepage";
import SlideFormModal from "./SlideFormModal";

export default function HeroSlidesManager({ slides, onRefresh, hasPermission }) {
  const [showModal, setShowModal] = useState(false);
  const [editing, setEditing] = useState(null);

  const handleCreate = async (data) => {
    try {
      await adminHeroSlides.create(data);
      toast.success("Slide created");
      setShowModal(false);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to create slide");
    }
  };

  const handleUpdate = async (data) => {
    try {
      await adminHeroSlides.update(editing.id, data);
      toast.success("Slide updated");
      setEditing(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update slide");
    }
  };

  const handleDelete = async (id) => {
    if (!confirm("Delete this slide?")) return;
    try {
      await adminHeroSlides.delete(id);
      toast.success("Slide deleted");
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to delete slide");
    }
  };

  const handleToggleActive = async (slide) => {
    try {
      await adminHeroSlides.update(slide.id, { is_active: !slide.is_active });
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update slide");
    }
  };

  const getStatusBadge = (slide) => {
    const now = new Date();
    if (!slide.is_active) return <span className="admin-badge badge-danger">Inactive</span>;
    if (slide.starts_at && new Date(slide.starts_at) > now) return <span className="admin-badge badge-warning">Scheduled</span>;
    if (slide.ends_at && new Date(slide.ends_at) < now) return <span className="admin-badge badge-danger">Expired</span>;
    return <span className="admin-badge badge-success">Active</span>;
  };

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 16 }}>
        <h3 style={{ margin: 0, fontSize: 16 }}>Hero Slides ({slides.length})</h3>
        {hasPermission && (
          <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal(true)}>
            + Add Slide
          </button>
        )}
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
        {slides.map((slide) => (
          <div key={slide.id} className="admin-card" style={{ padding: 12 }}>
            <div style={{ display: "flex", gap: 12, alignItems: "center" }}>
              <div style={{
                width: 100, height: 56, borderRadius: 6, overflow: "hidden",
                backgroundColor: "#f3f4f6", flexShrink: 0,
              }}>
                {slide.image_url && (
                  <img src={slide.image_url} alt={slide.image_alt || ""} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                )}
              </div>
              <div style={{ flex: 1 }}>
                <div style={{ fontWeight: 500, fontSize: 14 }}>{slide.title || "(No title)"}</div>
                <div style={{ fontSize: 12, color: "#6b7280" }}>{slide.subtitle}</div>
                <div style={{ display: "flex", gap: 8, marginTop: 4, alignItems: "center" }}>
                  {getStatusBadge(slide)}
                  <span style={{ fontSize: 11, color: "#9ca3af" }}>Order: {slide.sort_order}</span>
                  {slide.starts_at && <span style={{ fontSize: 11, color: "#9ca3af" }}>From: {new Date(slide.starts_at).toLocaleDateString()}</span>}
                  {slide.ends_at && <span style={{ fontSize: 11, color: "#9ca3af" }}>Until: {new Date(slide.ends_at).toLocaleDateString()}</span>}
                </div>
              </div>
              {hasPermission && (
                <div style={{ display: "flex", gap: 4, flexShrink: 0 }}>
                  <button
                    className="admin-btn btn-sm btn-secondary"
                    onClick={() => handleToggleActive(slide)}
                    title={slide.is_active ? "Deactivate" : "Activate"}
                  >
                    {slide.is_active ? "Deactivate" : "Activate"}
                  </button>
                  <button className="admin-btn btn-sm btn-secondary" onClick={() => setEditing(slide)}>Edit</button>
                  <button className="admin-btn btn-sm btn-danger" onClick={() => handleDelete(slide.id)}>Delete</button>
                </div>
              )}
            </div>
          </div>
        ))}
        {slides.length === 0 && (
          <div style={{ textAlign: "center", padding: 32, color: "#9ca3af" }}>
            No hero slides yet. Add your first slide to get started.
          </div>
        )}
      </div>

      {showModal && <SlideFormModal onSave={handleCreate} onClose={() => setShowModal(false)} />}
      {editing && <SlideFormModal slide={editing} onSave={handleUpdate} onClose={() => setEditing(null)} />}
    </div>
  );
}
