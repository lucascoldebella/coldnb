"use client";
import { useState, useEffect, useCallback } from "react";
import adminApi from "@/lib/adminApi";

function StatusBadge({ active }) {
  return (
    <span style={{
      display: "inline-flex", alignItems: "center",
      padding: "3px 10px", borderRadius: 20,
      fontSize: 12, fontWeight: 600,
      color: active ? "#16a34a" : "#6b7280",
      background: active ? "#f0fdf4" : "#f3f4f6",
    }}>
      {active ? "Active" : "Inactive"}
    </span>
  );
}

export default function AdminNewsletterPage() {
  const [subscribers, setSubscribers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [statusFilter, setStatusFilter] = useState("");
  const [deleting, setDeleting] = useState(null);

  const fetchSubscribers = useCallback(async () => {
    setLoading(true);
    try {
      const params = {};
      if (statusFilter) params.status = statusFilter;
      const res = await adminApi.get("/api/admin/newsletter/subscribers", { params });
      const data = res.data?.data?.subscribers || res.data?.subscribers || [];
      setSubscribers(data);
    } catch {
      setSubscribers([]);
    } finally {
      setLoading(false);
    }
  }, [statusFilter]);

  useEffect(() => { fetchSubscribers(); }, [fetchSubscribers]);

  const handleDelete = async (id) => {
    if (!confirm("Delete this subscriber?")) return;
    setDeleting(id);
    try {
      await adminApi.delete(`/api/admin/newsletter/subscribers/${id}`);
      fetchSubscribers();
    } catch (err) {
      alert(err.response?.data?.error || "Failed to delete");
    } finally {
      setDeleting(null);
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
      <div className="admin-card mb_20">
        <div className="card-header d-flex justify-content-between align-items-center">
          <div>
            <h5 className="card-title">Newsletter Subscribers</h5>
            {!loading && (
              <span className="text-secondary" style={{ fontSize: 13 }}>
                {subscribers.length} subscriber{subscribers.length !== 1 ? "s" : ""}
              </span>
            )}
          </div>
          <select
            className="form-control"
            style={{ width: "auto" }}
            value={statusFilter}
            onChange={(e) => setStatusFilter(e.target.value)}
          >
            <option value="">All</option>
            <option value="active">Active</option>
            <option value="inactive">Inactive</option>
          </select>
        </div>
        <div className="card-body">
          {loading ? (
            <div className="text-center py-4">
              <div className="spinner-border" role="status" />
            </div>
          ) : subscribers.length === 0 ? (
            <p className="text-secondary text-center py-4">No subscribers found.</p>
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Email</th>
                    <th>Name</th>
                    <th>Status</th>
                    <th>Subscribed</th>
                    <th>Action</th>
                  </tr>
                </thead>
                <tbody>
                  {subscribers.map((sub) => (
                    <tr key={sub.id}>
                      <td style={{ fontWeight: 600, fontSize: 13 }}>{sub.email}</td>
                      <td style={{ fontSize: 13 }}>{sub.name || "—"}</td>
                      <td><StatusBadge active={sub.is_active} /></td>
                      <td style={{ fontSize: 13 }}>{formatDate(sub.subscribed_at)}</td>
                      <td>
                        <button
                          className="btn btn-sm btn-outline-danger"
                          onClick={() => handleDelete(sub.id)}
                          disabled={deleting === sub.id}
                        >
                          {deleting === sub.id ? "..." : "Delete"}
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
    </div>
  );
}
