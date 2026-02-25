"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminCampaigns } from "@/lib/api/adminHomepage";
import CampaignFormModal from "./CampaignFormModal";

export default function CampaignsManager({ campaigns, heroSlides = [], banners = [], onRefresh, hasPermission }) {
  const [showModal, setShowModal] = useState(false);
  const [editing, setEditing] = useState(null);

  const handleCreate = async (data) => {
    try {
      await adminCampaigns.create(data);
      toast.success("Campaign created");
      setShowModal(false);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to create campaign");
    }
  };

  const handleUpdate = async (data) => {
    try {
      await adminCampaigns.update(editing.id, data);
      toast.success("Campaign updated");
      setEditing(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update campaign");
    }
  };

  const handleDelete = async (id) => {
    if (!confirm("Delete this campaign?")) return;
    try {
      await adminCampaigns.delete(id);
      toast.success("Campaign deleted");
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to delete campaign");
    }
  };

  const handleToggleActive = async (campaign) => {
    try {
      await adminCampaigns.update(campaign.id, { is_active: !campaign.is_active });
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update campaign");
    }
  };

  const getCampaignStatus = (campaign) => {
    const now = new Date();
    if (!campaign.is_active) return { label: "Inactive", className: "badge-danger" };
    if (campaign.starts_at && new Date(campaign.starts_at) > now) return { label: "Upcoming", className: "badge-warning" };
    if (campaign.ends_at && new Date(campaign.ends_at) < now) return { label: "Ended", className: "badge-danger" };
    return { label: "Active", className: "badge-success" };
  };

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 16 }}>
        <h3 style={{ margin: 0, fontSize: 16 }}>Campaigns ({campaigns.length})</h3>
        {hasPermission && (
          <button className="admin-btn btn-primary btn-sm" onClick={() => setShowModal(true)}>
            + Create Campaign
          </button>
        )}
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
        {campaigns.map((campaign) => {
          const status = getCampaignStatus(campaign);
          return (
            <div key={campaign.id} className="admin-card" style={{ padding: 14 }}>
              <div style={{ display: "flex", gap: 12, alignItems: "center" }}>
                <div style={{ flex: 1 }}>
                  <div style={{ fontWeight: 500, fontSize: 14 }}>{campaign.name}</div>
                  {campaign.description && (
                    <div style={{ fontSize: 12, color: "#6b7280", marginTop: 2 }}>{campaign.description}</div>
                  )}
                  <div style={{ display: "flex", gap: 8, marginTop: 6, alignItems: "center", flexWrap: "wrap" }}>
                    <span className={`admin-badge ${status.className}`}>{status.label}</span>
                    {campaign.starts_at && (
                      <span style={{ fontSize: 11, color: "#9ca3af" }}>
                        From: {new Date(campaign.starts_at).toLocaleDateString()}
                      </span>
                    )}
                    {campaign.ends_at && (
                      <span style={{ fontSize: 11, color: "#9ca3af" }}>
                        Until: {new Date(campaign.ends_at).toLocaleDateString()}
                      </span>
                    )}
                    {campaign.hero_slide_ids?.length > 0 && (
                      <span style={{ fontSize: 11, color: "#9ca3af" }}>
                        Slides: {campaign.hero_slide_ids.length}
                      </span>
                    )}
                    {campaign.banner_ids?.length > 0 && (
                      <span style={{ fontSize: 11, color: "#9ca3af" }}>
                        Banners: {campaign.banner_ids.length}
                      </span>
                    )}
                  </div>
                </div>
                {hasPermission && (
                  <div style={{ display: "flex", gap: 4, flexShrink: 0 }}>
                    <button className="admin-btn btn-sm btn-secondary" onClick={() => handleToggleActive(campaign)}>
                      {campaign.is_active ? "Deactivate" : "Activate"}
                    </button>
                    <button className="admin-btn btn-sm btn-secondary" onClick={() => setEditing(campaign)}>Edit</button>
                    <button className="admin-btn btn-sm btn-danger" onClick={() => handleDelete(campaign.id)}>Delete</button>
                  </div>
                )}
              </div>
            </div>
          );
        })}
        {campaigns.length === 0 && (
          <div style={{ textAlign: "center", padding: 32, color: "#9ca3af" }}>
            No campaigns yet. Create one to bundle hero slides and banners together.
          </div>
        )}
      </div>

      {showModal && (
        <CampaignFormModal
          heroSlides={heroSlides}
          banners={banners}
          onSave={handleCreate}
          onClose={() => setShowModal(false)}
        />
      )}
      {editing && (
        <CampaignFormModal
          campaign={editing}
          heroSlides={heroSlides}
          banners={banners}
          onSave={handleUpdate}
          onClose={() => setEditing(null)}
        />
      )}
    </div>
  );
}
