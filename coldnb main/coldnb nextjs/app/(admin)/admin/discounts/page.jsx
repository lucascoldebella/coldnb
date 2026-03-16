"use client";

import { useState, useEffect } from "react";
import { useAdmin } from "@/context/AdminContext";
import { adminDiscounts } from "@/lib/api/adminDiscounts";
import toast from "react-hot-toast";

const emptyForm = {
  code: "",
  description: "",
  discount_type: "percentage",
  discount_value: "",
  minimum_order: "0",
  maximum_discount: "",
  usage_limit: "",
  starts_at: "",
  expires_at: "",
  is_active: true,
};

export default function AdminDiscountsPage() {
  const { admin } = useAdmin();
  const [discounts, setDiscounts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [showModal, setShowModal] = useState(false);
  const [editing, setEditing] = useState(null);
  const [form, setForm] = useState(emptyForm);
  const [saving, setSaving] = useState(false);

  const fetchDiscounts = async () => {
    try {
      const res = await adminDiscounts.list();
      setDiscounts(res.data?.data || res.data || []);
    } catch (err) {
      toast.error("Failed to load discount codes");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchDiscounts();
  }, []);

  const openCreate = () => {
    setEditing(null);
    setForm(emptyForm);
    setShowModal(true);
  };

  const openEdit = (discount) => {
    setEditing(discount);
    setForm({
      code: discount.code || "",
      description: discount.description || "",
      discount_type: discount.discount_type || "percentage",
      discount_value: String(discount.discount_value || ""),
      minimum_order: String(discount.minimum_order || "0"),
      maximum_discount: String(discount.maximum_discount || ""),
      usage_limit: discount.usage_limit ? String(discount.usage_limit) : "",
      starts_at: discount.starts_at ? discount.starts_at.slice(0, 16) : "",
      expires_at: discount.expires_at ? discount.expires_at.slice(0, 16) : "",
      is_active: discount.is_active !== false,
    });
    setShowModal(true);
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!form.code || !form.discount_value) {
      toast.error("Code and value are required");
      return;
    }

    setSaving(true);
    try {
      const payload = {
        code: form.code.toUpperCase().trim(),
        description: form.description || null,
        discount_type: form.discount_type,
        discount_value: parseFloat(form.discount_value),
        minimum_order: parseFloat(form.minimum_order) || 0,
        maximum_discount: form.maximum_discount ? parseFloat(form.maximum_discount) : null,
        usage_limit: form.usage_limit ? parseInt(form.usage_limit) : null,
        starts_at: form.starts_at || null,
        expires_at: form.expires_at || null,
        is_active: form.is_active,
      };

      if (editing) {
        await adminDiscounts.update(editing.id, payload);
        toast.success("Discount code updated");
      } else {
        await adminDiscounts.create(payload);
        toast.success("Discount code created");
      }
      setShowModal(false);
      fetchDiscounts();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to save");
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async (id, code) => {
    if (!confirm(`Delete discount code "${code}"?`)) return;
    try {
      await adminDiscounts.delete(id);
      toast.success("Discount code deleted");
      fetchDiscounts();
    } catch {
      toast.error("Failed to delete");
    }
  };

  const toggleActive = async (discount) => {
    try {
      await adminDiscounts.update(discount.id, { is_active: !discount.is_active });
      fetchDiscounts();
    } catch {
      toast.error("Failed to update status");
    }
  };

  if (loading) {
    return (
      <div className="text-center py-5">
        <div className="spinner-border" role="status" />
      </div>
    );
  }

  return (
    <div>
      <div className="d-flex justify-content-between align-items-center mb-4">
        <h4 className="mb-0">Discount Codes</h4>
        <button className="btn btn-primary" onClick={openCreate}>
          + Create Discount
        </button>
      </div>

      <div className="admin-card">
        <div className="card-body p-0">
          <div className="table-responsive">
            <table className="table table-hover mb-0">
              <thead>
                <tr>
                  <th>Code</th>
                  <th>Type</th>
                  <th>Value</th>
                  <th>Min. Order</th>
                  <th>Usage</th>
                  <th>Status</th>
                  <th>Expires</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                {discounts.length === 0 ? (
                  <tr>
                    <td colSpan={8} className="text-center py-4 text-secondary">
                      No discount codes yet
                    </td>
                  </tr>
                ) : (
                  discounts.map((d) => (
                    <tr key={d.id}>
                      <td>
                        <strong>{d.code}</strong>
                        {d.description && (
                          <div className="text-secondary" style={{ fontSize: 12 }}>
                            {d.description}
                          </div>
                        )}
                      </td>
                      <td>
                        <span className={`badge ${d.discount_type === "percentage" ? "bg-info" : "bg-warning"}`}>
                          {d.discount_type === "percentage" ? "%" : "R$"}
                        </span>
                      </td>
                      <td>
                        {d.discount_type === "percentage"
                          ? `${d.discount_value}%`
                          : `R$ ${parseFloat(d.discount_value).toFixed(2)}`}
                      </td>
                      <td>R$ {parseFloat(d.minimum_order || 0).toFixed(2)}</td>
                      <td>
                        {d.used_count || 0}
                        {d.usage_limit ? ` / ${d.usage_limit}` : " / ~"}
                      </td>
                      <td>
                        <span
                          className={`badge ${d.is_active ? "bg-success" : "bg-secondary"}`}
                          style={{ cursor: "pointer" }}
                          onClick={() => toggleActive(d)}
                        >
                          {d.is_active ? "Active" : "Inactive"}
                        </span>
                      </td>
                      <td style={{ fontSize: 12 }}>
                        {d.expires_at
                          ? new Date(d.expires_at).toLocaleDateString()
                          : "Never"}
                      </td>
                      <td>
                        <button
                          className="btn btn-sm btn-outline-primary me-1"
                          onClick={() => openEdit(d)}
                        >
                          Edit
                        </button>
                        <button
                          className="btn btn-sm btn-outline-danger"
                          onClick={() => handleDelete(d.id, d.code)}
                        >
                          Delete
                        </button>
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      </div>

      {/* Create/Edit Modal */}
      {showModal && (
        <div
          className="modal d-block"
          style={{ backgroundColor: "rgba(0,0,0,0.5)" }}
          onClick={(e) => e.target === e.currentTarget && setShowModal(false)}
        >
          <div className="modal-dialog modal-lg modal-dialog-centered">
            <div className="modal-content">
              <div className="modal-header">
                <h5 className="modal-title">
                  {editing ? "Edit Discount Code" : "Create Discount Code"}
                </h5>
                <button
                  type="button"
                  className="btn-close"
                  onClick={() => setShowModal(false)}
                />
              </div>
              <form onSubmit={handleSubmit}>
                <div className="modal-body">
                  <div className="row g-3">
                    <div className="col-md-6">
                      <label className="form-label">Code *</label>
                      <input
                        className="form-control"
                        value={form.code}
                        onChange={(e) => setForm({ ...form, code: e.target.value })}
                        placeholder="SAVE10"
                        required
                      />
                    </div>
                    <div className="col-md-6">
                      <label className="form-label">Description</label>
                      <input
                        className="form-control"
                        value={form.description}
                        onChange={(e) => setForm({ ...form, description: e.target.value })}
                        placeholder="Summer sale discount"
                      />
                    </div>
                    <div className="col-md-4">
                      <label className="form-label">Type *</label>
                      <select
                        className="form-select"
                        value={form.discount_type}
                        onChange={(e) => setForm({ ...form, discount_type: e.target.value })}
                      >
                        <option value="percentage">Percentage (%)</option>
                        <option value="fixed">Fixed (R$)</option>
                      </select>
                    </div>
                    <div className="col-md-4">
                      <label className="form-label">Value *</label>
                      <input
                        type="number"
                        step="0.01"
                        className="form-control"
                        value={form.discount_value}
                        onChange={(e) => setForm({ ...form, discount_value: e.target.value })}
                        placeholder={form.discount_type === "percentage" ? "10" : "25.00"}
                        required
                      />
                    </div>
                    <div className="col-md-4">
                      <label className="form-label">Max Discount (R$)</label>
                      <input
                        type="number"
                        step="0.01"
                        className="form-control"
                        value={form.maximum_discount}
                        onChange={(e) => setForm({ ...form, maximum_discount: e.target.value })}
                        placeholder="50.00"
                      />
                    </div>
                    <div className="col-md-4">
                      <label className="form-label">Min. Order (R$)</label>
                      <input
                        type="number"
                        step="0.01"
                        className="form-control"
                        value={form.minimum_order}
                        onChange={(e) => setForm({ ...form, minimum_order: e.target.value })}
                      />
                    </div>
                    <div className="col-md-4">
                      <label className="form-label">Usage Limit</label>
                      <input
                        type="number"
                        className="form-control"
                        value={form.usage_limit}
                        onChange={(e) => setForm({ ...form, usage_limit: e.target.value })}
                        placeholder="Unlimited"
                      />
                    </div>
                    <div className="col-md-4 d-flex align-items-end">
                      <div className="form-check">
                        <input
                          type="checkbox"
                          className="form-check-input"
                          id="isActive"
                          checked={form.is_active}
                          onChange={(e) => setForm({ ...form, is_active: e.target.checked })}
                        />
                        <label className="form-check-label" htmlFor="isActive">
                          Active
                        </label>
                      </div>
                    </div>
                    <div className="col-md-6">
                      <label className="form-label">Starts At</label>
                      <input
                        type="datetime-local"
                        className="form-control"
                        value={form.starts_at}
                        onChange={(e) => setForm({ ...form, starts_at: e.target.value })}
                      />
                    </div>
                    <div className="col-md-6">
                      <label className="form-label">Expires At</label>
                      <input
                        type="datetime-local"
                        className="form-control"
                        value={form.expires_at}
                        onChange={(e) => setForm({ ...form, expires_at: e.target.value })}
                      />
                    </div>
                  </div>
                </div>
                <div className="modal-footer">
                  <button
                    type="button"
                    className="btn btn-secondary"
                    onClick={() => setShowModal(false)}
                  >
                    Cancel
                  </button>
                  <button type="submit" className="btn btn-primary" disabled={saving}>
                    {saving ? "Saving..." : editing ? "Update" : "Create"}
                  </button>
                </div>
              </form>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
