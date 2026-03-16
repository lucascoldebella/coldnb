"use client";
import { useState, useEffect, useCallback } from "react";
import adminApi from "@/lib/adminApi";

export default function AdminContactsPage() {
  const [submissions, setSubmissions] = useState([]);
  const [loading, setLoading] = useState(true);
  const [readFilter, setReadFilter] = useState("");
  const [selected, setSelected] = useState(null);

  const fetchSubmissions = useCallback(async () => {
    setLoading(true);
    try {
      const params = {};
      if (readFilter) params.is_read = readFilter;
      const res = await adminApi.get("/api/admin/contacts", { params });
      const data = res.data?.data?.submissions || res.data?.submissions || [];
      setSubmissions(data);
    } catch {
      setSubmissions([]);
    } finally {
      setLoading(false);
    }
  }, [readFilter]);

  useEffect(() => { fetchSubmissions(); }, [fetchSubmissions]);

  const handleMarkRead = async (id) => {
    try {
      await adminApi.put(`/api/admin/contacts/${id}/read`);
      setSubmissions((prev) =>
        prev.map((s) => (s.id === id ? { ...s, is_read: true } : s))
      );
      if (selected?.id === id) {
        setSelected((prev) => ({ ...prev, is_read: true }));
      }
    } catch (err) {
      alert(err.response?.data?.error || "Failed to update");
    }
  };

  const formatDate = (d) => {
    if (!d) return "—";
    return new Date(d).toLocaleDateString("en-US", {
      year: "numeric", month: "short", day: "numeric",
      hour: "2-digit", minute: "2-digit",
    });
  };

  const unreadCount = submissions.filter((s) => !s.is_read).length;

  return (
    <div className="admin-page-content">
      <div className="admin-card mb_20">
        <div className="card-header d-flex justify-content-between align-items-center">
          <div>
            <h5 className="card-title">Contact Submissions</h5>
            {!loading && unreadCount > 0 && (
              <span style={{
                fontSize: 12, fontWeight: 600,
                color: "#d97706", background: "#fffbeb",
                padding: "2px 8px", borderRadius: 12, marginLeft: 8,
              }}>
                {unreadCount} unread
              </span>
            )}
          </div>
          <select
            className="form-control"
            style={{ width: "auto" }}
            value={readFilter}
            onChange={(e) => setReadFilter(e.target.value)}
          >
            <option value="">All</option>
            <option value="false">Unread</option>
            <option value="true">Read</option>
          </select>
        </div>
        <div className="card-body">
          {loading ? (
            <div className="text-center py-4">
              <div className="spinner-border" role="status" />
            </div>
          ) : submissions.length === 0 ? (
            <p className="text-secondary text-center py-4">No submissions found.</p>
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th style={{ width: 8 }}></th>
                    <th>Name</th>
                    <th>Email</th>
                    <th>Subject</th>
                    <th>Date</th>
                    <th>Action</th>
                  </tr>
                </thead>
                <tbody>
                  {submissions.map((sub) => (
                    <tr key={sub.id} style={{ fontWeight: sub.is_read ? 400 : 600 }}>
                      <td>
                        {!sub.is_read && (
                          <span style={{
                            display: "inline-block", width: 8, height: 8,
                            borderRadius: "50%", background: "#1d4ed8",
                          }} />
                        )}
                      </td>
                      <td style={{ fontSize: 13 }}>{sub.name}</td>
                      <td style={{ fontSize: 13 }}>{sub.email}</td>
                      <td style={{ fontSize: 13 }}>{sub.subject || "—"}</td>
                      <td style={{ fontSize: 13 }}>{formatDate(sub.created_at)}</td>
                      <td>
                        <button
                          className="btn btn-sm btn-outline-primary"
                          onClick={() => setSelected(sub)}
                        >
                          View
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

      {/* Message Detail Modal */}
      {selected && (
        <div className="modal fade show d-block" style={{ background: "rgba(0,0,0,0.5)" }}>
          <div className="modal-dialog modal-dialog-centered modal-lg">
            <div className="modal-content">
              <div className="modal-header">
                <h5 className="modal-title">
                  Message from {selected.name}
                </h5>
                <button
                  className="btn-close"
                  onClick={() => setSelected(null)}
                />
              </div>
              <div className="modal-body">
                <div className="row mb_12">
                  <div className="col-sm-6">
                    <div className="text-secondary" style={{ fontSize: 12 }}>Email</div>
                    <div style={{ fontSize: 14 }}>{selected.email}</div>
                  </div>
                  <div className="col-sm-3">
                    <div className="text-secondary" style={{ fontSize: 12 }}>Phone</div>
                    <div style={{ fontSize: 14 }}>{selected.phone || "—"}</div>
                  </div>
                  <div className="col-sm-3">
                    <div className="text-secondary" style={{ fontSize: 12 }}>Date</div>
                    <div style={{ fontSize: 14 }}>{formatDate(selected.created_at)}</div>
                  </div>
                </div>
                {selected.subject && (
                  <div className="mb_12">
                    <div className="text-secondary" style={{ fontSize: 12 }}>Subject</div>
                    <div style={{ fontSize: 14, fontWeight: 600 }}>{selected.subject}</div>
                  </div>
                )}
                <div>
                  <div className="text-secondary" style={{ fontSize: 12 }}>Message</div>
                  <div style={{
                    fontSize: 14, whiteSpace: "pre-wrap",
                    background: "#f8f9fa", padding: 16, borderRadius: 8, marginTop: 4,
                  }}>
                    {selected.message}
                  </div>
                </div>
              </div>
              <div className="modal-footer">
                {!selected.is_read && (
                  <button
                    className="btn btn-primary"
                    onClick={() => handleMarkRead(selected.id)}
                  >
                    Mark as Read
                  </button>
                )}
                <button
                  className="btn btn-secondary"
                  onClick={() => setSelected(null)}
                >
                  Close
                </button>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
