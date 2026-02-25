"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminBanners } from "@/lib/api/adminHomepage";
import BannerFormModal from "./BannerFormModal";

export default function CountdownBannerManager({ banners, onRefresh, hasPermission }) {
  const [showModal, setShowModal] = useState(false);
  const [editing, setEditing] = useState(null);

  const countdownBanners = banners.filter((b) => b.banner_type === "countdown");

  const handleCreate = async (data) => {
    try {
      await adminBanners.create({ ...data, banner_type: "countdown" });
      toast.success("Countdown banner created");
      setShowModal(false);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to create banner");
    }
  };

  const handleUpdate = async (data) => {
    try {
      await adminBanners.update(editing.id, data);
      toast.success("Banner updated");
      setEditing(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update banner");
    }
  };

  const handleDelete = async (id) => {
    if (!confirm("Delete this banner?")) return;
    try {
      await adminBanners.delete(id);
      toast.success("Banner deleted");
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to delete banner");
    }
  };

  const handleToggleActive = async (banner) => {
    try {
      await adminBanners.update(banner.id, { is_active: !banner.is_active });
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update banner");
    }
  };

  const activeBanner = countdownBanners.find((b) => b.is_active);

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)" }}>
          Full-width banner with countdown timer
        </div>
        {hasPermission && (
          <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal(true)}>
            + Add Countdown Banner
          </button>
        )}
      </div>

      {countdownBanners.length > 0 ? (
        <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
          {countdownBanners.map((banner) => (
            <div key={banner.id} style={{
              display: "flex", gap: 12, alignItems: "center",
              border: "1px solid var(--admin-border)",
              borderRadius: 8, padding: 12,
            }}>
              <div style={{
                width: 120, height: 68, borderRadius: 6, overflow: "hidden",
                backgroundColor: "#f3f4f6", flexShrink: 0,
              }}>
                {banner.image_url && (
                  <img src={banner.image_url} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                )}
              </div>
              <div style={{ flex: 1 }}>
                <div style={{ fontWeight: 500, fontSize: 14 }}>{banner.title || "(No title)"}</div>
                {banner.discount_label && (
                  <div style={{ fontSize: 12, color: "var(--admin-primary)", fontWeight: 500 }}>{banner.discount_label}</div>
                )}
                <div style={{ display: "flex", gap: 8, marginTop: 4, alignItems: "center" }}>
                  <span className={`admin-badge ${banner.is_active ? "badge-success" : "badge-danger"}`}>
                    {banner.is_active ? "Active" : "Inactive"}
                  </span>
                  {banner.countdown_end_at && (
                    <span style={{ fontSize: 11, color: "#9ca3af" }}>
                      Ends: {new Date(banner.countdown_end_at).toLocaleString()}
                    </span>
                  )}
                </div>
              </div>
              {hasPermission && (
                <div style={{ display: "flex", gap: 4, flexShrink: 0 }}>
                  <button className="admin-btn btn-sm btn-secondary" onClick={() => handleToggleActive(banner)}>
                    {banner.is_active ? "Deactivate" : "Activate"}
                  </button>
                  <button className="admin-btn btn-sm btn-secondary" onClick={() => setEditing(banner)}>Edit</button>
                  <button className="admin-btn btn-sm btn-danger" onClick={() => handleDelete(banner.id)}>Delete</button>
                </div>
              )}
            </div>
          ))}
        </div>
      ) : (
        <div style={{ textAlign: "center", padding: 20, color: "#9ca3af", fontSize: 13 }}>
          No countdown banner configured.
        </div>
      )}

      {showModal && (
        <BannerFormModal bannerType="countdown" onSave={handleCreate} onClose={() => setShowModal(false)} />
      )}
      {editing && (
        <BannerFormModal banner={editing} bannerType="countdown" onSave={handleUpdate} onClose={() => setEditing(null)} />
      )}
    </div>
  );
}
