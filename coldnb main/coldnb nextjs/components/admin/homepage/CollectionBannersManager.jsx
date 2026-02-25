"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminBanners } from "@/lib/api/adminHomepage";
import BannerFormModal from "./BannerFormModal";

export default function CollectionBannersManager({ banners, onRefresh, hasPermission }) {
  const [showModal, setShowModal] = useState(false);
  const [editing, setEditing] = useState(null);

  const collectionBanners = banners.filter((b) => b.banner_type === "collection");

  const handleCreate = async (data) => {
    try {
      await adminBanners.create({ ...data, banner_type: "collection" });
      toast.success("Collection banner created");
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

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)" }}>
          Two side-by-side banners shown below the products section
        </div>
        {hasPermission && (
          <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal(true)}>
            + Add Banner
          </button>
        )}
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
        {collectionBanners.map((banner) => (
          <div key={banner.id} style={{
            border: "1px solid var(--admin-border)",
            borderRadius: 8,
            overflow: "hidden",
          }}>
            <div style={{
              width: "100%", height: 100, backgroundColor: "#f3f4f6",
            }}>
              {banner.image_url && (
                <img src={banner.image_url} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />
              )}
            </div>
            <div style={{ padding: 10 }}>
              <div style={{ fontWeight: 500, fontSize: 13 }}>{banner.title || "(No title)"}</div>
              <div style={{ fontSize: 11, color: "#6b7280" }}>{banner.subtitle}</div>
              <div style={{ display: "flex", gap: 6, marginTop: 6, alignItems: "center" }}>
                <span className={`admin-badge ${banner.is_active ? "badge-success" : "badge-danger"}`}>
                  {banner.is_active ? "Active" : "Inactive"}
                </span>
                <span style={{ fontSize: 10, color: "#9ca3af" }}>Pos: {banner.position}</span>
              </div>
              {hasPermission && (
                <div style={{ display: "flex", gap: 4, marginTop: 8 }}>
                  <button className="admin-btn btn-sm btn-secondary" onClick={() => handleToggleActive(banner)}>
                    {banner.is_active ? "Deactivate" : "Activate"}
                  </button>
                  <button className="admin-btn btn-sm btn-secondary" onClick={() => setEditing(banner)}>Edit</button>
                  <button className="admin-btn btn-sm btn-danger" onClick={() => handleDelete(banner.id)}>Delete</button>
                </div>
              )}
            </div>
          </div>
        ))}
      </div>

      {collectionBanners.length === 0 && (
        <div style={{ textAlign: "center", padding: 20, color: "#9ca3af", fontSize: 13 }}>
          No collection banners. Add a pair for the left/right layout.
        </div>
      )}

      {showModal && (
        <BannerFormModal bannerType="collection" onSave={handleCreate} onClose={() => setShowModal(false)} />
      )}
      {editing && (
        <BannerFormModal banner={editing} bannerType="collection" onSave={handleUpdate} onClose={() => setEditing(null)} />
      )}
    </div>
  );
}
