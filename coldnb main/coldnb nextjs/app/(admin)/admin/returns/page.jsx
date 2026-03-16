"use client";
import { useState, useEffect, useCallback } from "react";
import adminApi from "@/lib/adminApi";

const STATUS_COLORS = {
  requested:    { color: "#1d4ed8", bg: "#eff6ff", label: "Requested" },
  under_review: { color: "#d97706", bg: "#fffbeb", label: "Under Review" },
  approved:     { color: "#16a34a", bg: "#f0fdf4", label: "Approved" },
  rejected:     { color: "#dc2626", bg: "#fef2f2", label: "Rejected" },
  refunded:     { color: "#7c3aed", bg: "#f5f3ff", label: "Refunded" },
};

const REASON_LABELS = {
  defective:    "Defective / Damaged",
  wrong_item:   "Wrong Item Received",
  changed_mind: "Changed Mind",
  other:        "Other",
};

function StatusBadge({ status }) {
  const s = STATUS_COLORS[status] || { color: "#6b7280", bg: "#f3f4f6", label: status };
  return (
    <span style={{
      display: "inline-flex", alignItems: "center",
      padding: "3px 10px", borderRadius: 20,
      fontSize: 12, fontWeight: 600,
      color: s.color, background: s.bg,
    }}>
      {s.label}
    </span>
  );
}

export default function AdminReturnsPage() {
  const [returns, setReturns] = useState([]);
  const [loading, setLoading] = useState(true);
  const [statusFilter, setStatusFilter] = useState("");
  const [selected, setSelected] = useState(null);
  const [modalOpen, setModalOpen] = useState(false);
  const [updateStatus, setUpdateStatus] = useState("");
  const [adminNotes, setAdminNotes] = useState("");
  const [refundAmount, setRefundAmount] = useState("");
  const [saving, setSaving] = useState(false);

  const fetchReturns = useCallback(async () => {
    setLoading(true);
    try {
      const params = {};
      if (statusFilter) params.status = statusFilter;
      const res = await adminApi.get("/api/admin/returns", { params });
      const data = res.data?.data?.returns || res.data?.returns || [];
      setReturns(data);
    } catch {
      setReturns([]);
    } finally {
      setLoading(false);
    }
  }, [statusFilter]);

  useEffect(() => { fetchReturns(); }, [fetchReturns]);

  const openModal = (ret) => {
    setSelected(ret);
    setUpdateStatus(ret.status);
    setAdminNotes(ret.admin_notes || "");
    setRefundAmount(ret.refund_amount ? String(ret.refund_amount) : "");
    setModalOpen(true);
  };

  const handleSave = async () => {
    if (!selected) return;
    setSaving(true);
    try {
      await adminApi.put(`/api/admin/returns/${selected.id}/status`, {
        status: updateStatus,
        admin_notes: adminNotes || null,
        refund_amount: refundAmount ? parseFloat(refundAmount) : 0,
      });
      setModalOpen(false);
      fetchReturns();
    } catch (err) {
      alert(err.response?.data?.error || "Failed to update return");
    } finally {
      setSaving(false);
    }
  };

  const formatDate = (d) => {
    if (!d) return "—";
    return new Date(d).toLocaleDateString("en-US", {
      year: "numeric", month: "short", day: "numeric",
    });
  };

  return (
    <div className="admin-page-content">
      {/* Header */}
      <div className="admin-card mb_20">
        <div className="card-header d-flex justify-content-between align-items-center">
          <h5 className="card-title">Returns & Refunds</h5>
          <select
            className="form-control"
            style={{ width: "auto" }}
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
          >
            <option value="">All Statuses</option>
            {Object.entries(STATUS_COLORS).map(([key, val]) => (
              <option key={key} value={key}>{val.label}</option>
            ))}
          </select>
        </div>
        <div className="card-body">
          {loading ? (
            <div className="text-center py-4">
              <div className="spinner-border" role="status" />
            </div>
          ) : returns.length === 0 ? (
            <p className="text-secondary text-center py-4">No return requests found.</p>
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Order #</th>
                    <th>Customer</th>
                    <th>Reason</th>
                    <th>Status</th>
                    <th>Refund</th>
                    <th>Date</th>
                    <th>Action</th>
                  </tr>
                </thead>
                <tbody>
                  {returns.map((ret) => (
                    <tr key={ret.id}>
                      <td>
                        <a
                          href={`/admin/orders/${ret.order_id}`}
                          className="link"
                          style={{ fontWeight: 600 }}
                        >
                          #{ret.order_number}
                        </a>
                      </td>
                      <td>
                        <div style={{ fontSize: 13 }}>
                          <div className="fw-6">{ret.customer_name || "—"}</div>
                          <div className="text-secondary">{ret.customer_email || "—"}</div>
                        </div>
                      </td>
                      <td style={{ fontSize: 13 }}>
                        {REASON_LABELS[ret.reason] || ret.reason}
                      </td>
                      <td><StatusBadge status={ret.status} /></td>
                      <td style={{ fontSize: 13 }}>
                        {ret.refund_amount
                          ? `R$ ${parseFloat(ret.refund_amount).toFixed(2)}`
                          : "—"}
                      </td>
                      <td style={{ fontSize: 13 }}>{formatDate(ret.created_at)}</td>
                      <td>
                        <button
                          className="btn btn-sm btn-outline-primary"
                          onClick={() => openModal(ret)}
                        >
                          Review
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>

      {/* Review Modal */}
      {modalOpen && selected && (
        <div className="modal fade show d-block" style={{ background: "rgba(0,0,0,0.5)" }}>
          <div className="modal-dialog modal-dialog-centered">
            <div className="modal-content">
              <div className="modal-header">
                <h5 className="modal-title">
                  Review Return — #{selected.order_number}
                </h5>
                <button
                  className="btn-close"
                  onClick={() => setModalOpen(false)}
                />
              </div>
              <div className="modal-body">
                <div className="mb_16">
                  <p className="text-secondary" style={{ fontSize: 13 }}>
                    <strong>Reason:</strong> {REASON_LABELS[selected.reason] || selected.reason}
                  </p>
                  {selected.description && (
                    <p className="text-secondary" style={{ fontSize: 13 }}>
                      <strong>Description:</strong> {selected.description}
                    </p>
                  )}
                </div>
                <div className="form-group mb_12">
                  <label className="form-label">Status</label>
                  <select
                    className="form-control"
                    value={updateStatus}
                    onChange={(e) => setUpdateStatus(e.target.value)}
                  >
                    {Object.entries(STATUS_COLORS).map(([key, val]) => (
                      <option key={key} value={key}>{val.label}</option>
                    ))}
                  </select>
                </div>
                <div className="form-group mb_12">
                  <label className="form-label">Refund Amount (R$)</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="0.00"
                    value={refundAmount}
                    onChange={(e) => setRefundAmount(e.target.value)}
                    min="0"
                    step="0.01"
                  />
                </div>
                <div className="form-group">
                  <label className="form-label">Admin Notes</label>
                  <textarea
                    className="form-control"
                    rows={3}
                    placeholder="Internal notes about this return..."
                    value={adminNotes}
                    onChange={(e) => setAdminNotes(e.target.value)}
                  />
                </div>
              </div>
              <div className="modal-footer">
                <button
                  className="btn btn-secondary"
                  onClick={() => setModalOpen(false)}
                >
                  Cancel
                </button>
                <button
                  className="btn btn-primary"
                  onClick={handleSave}
                  disabled={saving}
                >
                  {saving ? "Saving..." : "Save Changes"}
                </button>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
