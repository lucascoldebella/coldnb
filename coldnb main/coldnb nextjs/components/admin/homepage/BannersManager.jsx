"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminBanners } from "@/lib/api/adminHomepage";
import BannerFormModal from "./BannerFormModal";

export default function BannersManager({ banners, onRefresh, hasPermission }) {
  const [showModal, setShowModal] = useState(null); // null | "collection" | "countdown"
  const [editing, setEditing] = useState(null);

  const collectionBanners = banners.filter((b) => b.banner_type === "collection");
  const countdownBanners = banners.filter((b) => b.banner_type === "countdown");

  const handleCreate = async (data) => {
    try {
      await adminBanners.create(data);
      toast.success("Banner created");
      setShowModal(null);
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

  const renderBannerCard = (banner) => (
    <div key={banner.id} className="admin-card" style={{ padding: 12 }}>
      <div style={{ display: "flex", gap: 12, alignItems: "center" }}>
        <div style={{
          width: 80, height: 56, borderRadius: 6, overflow: "hidden",
          backgroundColor: "#f3f4f6", flexShrink: 0,
        }}>
          {banner.image_url && (
            <img src={banner.image_url} alt="" style={{ width: "100%", height: "100%", objectFit: "cover" }} />
          )}
        </div>
        <div style={{ flex: 1 }}>
          <div style={{ fontWeight: 500, fontSize: 14 }}>{banner.title || "(No title)"}</div>
          <div style={{ fontSize: 12, color: "#6b7280" }}>{banner.subtitle}</div>
          <div style={{ display: "flex", gap: 8, marginTop: 4, alignItems: "center" }}>
            <span className={`admin-badge ${banner.is_active ? "badge-success" : "badge-danger"}`}>
              {banner.is_active ? "Active" : "Inactive"}
            </span>
            {banner.banner_type === "collection" && (
              <>
                <span style={{ fontSize: 11, color: "#9ca3af" }}>Color: {banner.text_color}</span>
                <span style={{ fontSize: 11, color: "#9ca3af" }}>Pos: {banner.position}</span>
              </>
            )}
            {banner.banner_type === "countdown" && banner.countdown_end_at && (
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
    </div>
  );

  return (
    <div>
      <div style={{ marginBottom: 24 }}>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
          <h3 style={{ margin: 0, fontSize: 16 }}>Collection Banners ({collectionBanners.length})</h3>
          {hasPermission && (
            <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal("collection")}>
              + Add Collection Banner
            </button>
          )}
        </div>
        <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
          {collectionBanners.map(renderBannerCard)}
          {collectionBanners.length === 0 && (
            <div style={{ textAlign: "center", padding: 20, color: "#9ca3af", fontSize: 13 }}>
              No collection banners. Add a pair for the left/right layout.
            </div>
          )}
        </div>
      </div>

      <div>
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
          <h3 style={{ margin: 0, fontSize: 16 }}>Countdown Banner</h3>
          {hasPermission && (
            <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal("countdown")}>
              + Add Countdown Banner
            </button>
          )}
        </div>
        <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
          {countdownBanners.map(renderBannerCard)}
          {countdownBanners.length === 0 && (
            <div style={{ textAlign: "center", padding: 20, color: "#9ca3af", fontSize: 13 }}>
              No countdown banner configured.
            </div>
          )}
        </div>
      </div>

      {showModal && (
        <BannerFormModal
          bannerType={showModal}
          onSave={handleCreate}
          onClose={() => setShowModal(null)}
        />
      )}
      {editing && (
        <BannerFormModal
          banner={editing}
          bannerType={editing.banner_type}
          onSave={handleUpdate}
          onClose={() => setEditing(null)}
        />
      )}
    </div>
  );
}
