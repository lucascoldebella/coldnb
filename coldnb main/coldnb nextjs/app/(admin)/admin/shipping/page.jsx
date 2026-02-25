"use client";
import { useState, useEffect, useMemo, useCallback } from "react";
import { useAdmin } from "@/context/AdminContext";
import { adminShipping } from "@/lib/api/adminShipping";
import toast from "react-hot-toast";
import DataTable from "@/components/admin/tables/DataTable";

const PlusIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="5" x2="12" y2="19" />
    <line x1="5" y1="12" x2="19" y2="12" />
  </svg>
);

const EditIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
    <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
  </svg>
);

const TrashIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="3 6 5 6 21 6" />
    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" />
  </svg>
);

const XIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" />
    <line x1="6" y1="6" x2="18" y2="18" />
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
  fontSize: 11, fontWeight: 600, textTransform: "uppercase",
  letterSpacing: "0.05em", color: "var(--admin-primary)",
  marginBottom: 12, display: "flex", alignItems: "center", gap: 6,
};

const defaultForm = {
  name: "",
  min_distance_km: "0",
  max_distance_km: "100",
  price: "0",
  estimated_days_min: "1",
  estimated_days_max: "3",
  is_active: true,
  sort_order: "0",
};

export default function ShippingPage() {
  const { hasPermission } = useAdmin();
  const [loading, setLoading] = useState(true);
  const [zones, setZones] = useState([]);
  const [showModal, setShowModal] = useState(false);
  const [editingZone, setEditingZone] = useState(null);
  const [saving, setSaving] = useState(false);
  const [deleteConfirm, setDeleteConfirm] = useState(null);
  const [formData, setFormData] = useState(defaultForm);

  const fetchZones = async () => {
    try {
      const res = await adminShipping.listZones();
      const data = res.data?.data || res.data || [];
      setZones(Array.isArray(data) ? data : []);
    } catch (error) {
      toast.error("Failed to load shipping zones");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchZones();
  }, []);

  const openModal = useCallback((zone = null) => {
    if (zone) {
      setEditingZone(zone);
      setFormData({
        name: zone.name,
        min_distance_km: String(zone.min_distance_km ?? 0),
        max_distance_km: String(zone.max_distance_km ?? 100),
        price: String(parseFloat(zone.price) || 0),
        estimated_days_min: String(zone.estimated_days_min ?? 1),
        estimated_days_max: String(zone.estimated_days_max ?? 3),
        is_active: zone.is_active !== false,
        sort_order: String(zone.sort_order ?? 0),
      });
    } else {
      setEditingZone(null);
      setFormData(defaultForm);
    }
    setShowModal(true);
  }, []);

  const closeModal = () => {
    setShowModal(false);
    setEditingZone(null);
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!formData.name.trim()) {
      toast.error("Zone name is required");
      return;
    }
    setSaving(true);
    try {
      const payload = {
        name: formData.name,
        min_distance_km: parseInt(formData.min_distance_km) || 0,
        max_distance_km: parseInt(formData.max_distance_km) || 0,
        price: parseFloat(formData.price) || 0,
        estimated_days_min: parseInt(formData.estimated_days_min) || 1,
        estimated_days_max: parseInt(formData.estimated_days_max) || 1,
        is_active: formData.is_active,
        sort_order: parseInt(formData.sort_order) || 0,
      };
      if (editingZone) {
        await adminShipping.updateZone(editingZone.id, payload);
        toast.success("Shipping zone updated");
      } else {
        await adminShipping.createZone(payload);
        toast.success("Shipping zone created");
      }
      closeModal();
      fetchZones();
    } catch (error) {
      const msg = error.response?.data?.error;
      toast.error(typeof msg === "string" ? msg : "Failed to save shipping zone");
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async (id) => {
    try {
      await adminShipping.deleteZone(id);
      toast.success("Shipping zone deleted");
      setDeleteConfirm(null);
      fetchZones();
    } catch (error) {
      toast.error("Failed to delete shipping zone");
    }
  };

  const columns = useMemo(() => [
    {
      header: "Name",
      accessor: "name",
      render: (row) => <strong>{row.name}</strong>,
    },
    {
      header: "Distance Range (km)",
      accessor: "min_distance_km",
      render: (row) => `${row.min_distance_km} - ${row.max_distance_km}`,
    },
    {
      header: "Price",
      accessor: "price",
      render: (row) => `R$ ${parseFloat(row.price).toFixed(2)}`,
    },
    {
      header: "Est. Days",
      accessor: "estimated_days_min",
      render: (row) => `${row.estimated_days_min} - ${row.estimated_days_max}`,
    },
    {
      header: "Active",
      accessor: "is_active",
      render: (row) => (
        <span className={`admin-badge ${row.is_active !== false ? "badge-success" : "badge-secondary"}`}>
          {row.is_active !== false ? "Active" : "Inactive"}
        </span>
      ),
    },
    {
      header: "Actions",
      accessor: "actions",
      sortable: false,
      width: "100px",
      className: "cell-actions",
      render: (row) => (
        <div className="table-row-actions">
          <button className="action-btn action-edit" onClick={(e) => { e.stopPropagation(); openModal(row); }}>
            <EditIcon />
          </button>
          <button className="action-btn action-delete" onClick={(e) => { e.stopPropagation(); setDeleteConfirm(row.id); }}>
            <TrashIcon />
          </button>
        </div>
      ),
    },
  ], [openModal]);

  return (
    <div className="shipping-page">
      <div className="admin-page-header">
        <h1 className="page-title">Shipping Zones</h1>
        <div className="page-actions">
          {hasPermission("create_products") && (
            <button className="admin-btn btn-primary" onClick={() => openModal()}>
              <PlusIcon />
              Add Zone
            </button>
          )}
        </div>
      </div>

      <DataTable
        columns={columns}
        data={zones}
        loading={loading}
        searchPlaceholder="Search zones..."
        pagination={false}
      />

      {/* Add/Edit Modal */}
      {showModal && (
        <div className="admin-modal-overlay open">
          <div className="admin-modal md" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3 className="modal-title">
                {editingZone ? "Edit Shipping Zone" : "Add Shipping Zone"}
              </h3>
              <button className="modal-close" onClick={closeModal}>
                <XIcon />
              </button>
            </div>
            <form
              onSubmit={handleSubmit}
              style={{ display: "flex", flexDirection: "column", overflow: "hidden", flex: 1 }}
            >
              <div className="modal-body" style={{ flex: 1, overflowY: "auto" }}>
                <div style={fieldGroupStyle}>
                  <div style={sectionLabelStyle}>Zone Details</div>
                  <div className="form-group">
                    <label>Zone Name</label>
                    <input
                      className="admin-input"
                      value={formData.name}
                      onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                      placeholder="e.g., Local, Regional, Interstate"
                      required
                    />
                  </div>
                  <div className="form-row cols-2">
                    <div className="form-group">
                      <label>Min Distance (km)</label>
                      <input
                        type="number"
                        className="admin-input"
                        value={formData.min_distance_km}
                        onChange={(e) => setFormData({ ...formData, min_distance_km: e.target.value })}
                        min="0"
                        required
                      />
                    </div>
                    <div className="form-group">
                      <label>Max Distance (km)</label>
                      <input
                        type="number"
                        className="admin-input"
                        value={formData.max_distance_km}
                        onChange={(e) => setFormData({ ...formData, max_distance_km: e.target.value })}
                        min="0"
                        required
                      />
                    </div>
                  </div>
                </div>

                <div style={fieldGroupStyle}>
                  <div style={sectionLabelStyle}>Pricing & Delivery</div>
                  <div className="form-group">
                    <label>Price (R$)</label>
                    <input
                      type="number"
                      step="0.01"
                      className="admin-input"
                      value={formData.price}
                      onChange={(e) => setFormData({ ...formData, price: e.target.value })}
                      min="0"
                      required
                    />
                  </div>
                  <div className="form-row cols-2">
                    <div className="form-group">
                      <label>Min Delivery Days</label>
                      <input
                        type="number"
                        className="admin-input"
                        value={formData.estimated_days_min}
                        onChange={(e) => setFormData({ ...formData, estimated_days_min: e.target.value })}
                        min="1"
                        required
                      />
                    </div>
                    <div className="form-group">
                      <label>Max Delivery Days</label>
                      <input
                        type="number"
                        className="admin-input"
                        value={formData.estimated_days_max}
                        onChange={(e) => setFormData({ ...formData, estimated_days_max: e.target.value })}
                        min="1"
                        required
                      />
                    </div>
                  </div>
                </div>

                <div style={fieldGroupStyle}>
                  <div style={sectionLabelStyle}>Settings</div>
                  <div className="form-row cols-2">
                    <div className="form-group">
                      <label>Sort Order</label>
                      <input
                        type="number"
                        className="admin-input"
                        value={formData.sort_order}
                        onChange={(e) => setFormData({ ...formData, sort_order: e.target.value })}
                      />
                    </div>
                    <div className="form-group" style={{ display: "flex", alignItems: "center", paddingTop: 24 }}>
                      <label className="admin-toggle">
                        <span
                          className={`toggle-switch ${formData.is_active ? "active" : ""}`}
                          onClick={() => setFormData({ ...formData, is_active: !formData.is_active })}
                        />
                        <span className="toggle-label">
                          {formData.is_active ? "Active" : "Inactive"}
                        </span>
                      </label>
                    </div>
                  </div>
                </div>
              </div>
              <div className="modal-footer">
                <button type="button" className="admin-btn btn-secondary" onClick={closeModal}>
                  Cancel
                </button>
                <button type="submit" className="admin-btn btn-primary" disabled={saving}>
                  {saving ? "Saving..." : editingZone ? "Update Zone" : "Create Zone"}
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* Delete Confirmation Modal */}
      {deleteConfirm && (
        <div className="admin-modal-overlay open">
          <div className="admin-modal sm" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3 className="modal-title">Delete Shipping Zone</h3>
              <button className="modal-close" onClick={() => setDeleteConfirm(null)}>
                <XIcon />
              </button>
            </div>
            <div className="modal-body">
              <p>Are you sure you want to delete this shipping zone? This action cannot be undone.</p>
            </div>
            <div className="modal-footer">
              <button className="admin-btn btn-secondary" onClick={() => setDeleteConfirm(null)}>
                Cancel
              </button>
              <button className="admin-btn btn-danger" onClick={() => handleDelete(deleteConfirm)}>
                Delete
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
