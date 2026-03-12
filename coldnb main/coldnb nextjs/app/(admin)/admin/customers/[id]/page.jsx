"use client";
import { useState, useEffect } from "react";
import { useRouter, useParams } from "next/navigation";
import Link from "next/link";
import adminApi from "@/lib/adminApi";
import StatusBadge from "@/components/admin/orders/StatusBadge";
import toast from "react-hot-toast";
import { formatCurrency, formatDate } from "@/lib/adminUtils";

const ArrowLeftIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="19" y1="12" x2="5" y2="12" />
    <polyline points="12 19 5 12 12 5" />
  </svg>
);

export default function CustomerDetailPage() {
  const router = useRouter();
  const params = useParams();
  const [loading, setLoading] = useState(true);
  const [customer, setCustomer] = useState(null);
  const [orders, setOrders] = useState([]);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [userRes, ordersRes] = await Promise.all([
          adminApi.get(`/api/admin/users/${params.id}`),
          adminApi.get("/api/admin/orders", { params: { search: params.id, limit: 50 } }),
        ]);
        const userData = userRes.data?.data || userRes.data;
        setCustomer(userData);

        const ordersData = ordersRes.data?.data?.orders || ordersRes.data?.orders || [];
        // Filter orders to only show this customer's orders
        const customerOrders = ordersData.filter(o =>
          o.customer_email === userData.email
        );
        setOrders(customerOrders);
      } catch {
        toast.error("Failed to load customer details");
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, [params.id]);

  if (loading) {
    return (
      <div className="customer-detail-page">
        <div className="admin-page-header">
          <div className="skeleton skeleton-title" style={{ width: 200 }} />
        </div>
        <div className="admin-card">
          <div className="card-body">
            <div className="skeleton skeleton-text" style={{ marginBottom: 16 }} />
            <div className="skeleton skeleton-text" />
          </div>
        </div>
      </div>
    );
  }

  if (!customer) {
    return (
      <div className="customer-detail-page">
        <div className="admin-page-header">
          <h1 className="page-title">Customer Not Found</h1>
        </div>
      </div>
    );
  }

  const initials = (customer.full_name || customer.email || "?")
    .split(" ")
    .map(w => w[0])
    .join("")
    .toUpperCase()
    .slice(0, 2);

  return (
    <div className="customer-detail-page">
      <div className="admin-page-header">
        <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
          <button
            className="admin-btn btn-ghost btn-icon"
            onClick={() => router.back()}
          >
            <ArrowLeftIcon />
          </button>
          <h1 className="page-title">Customer Details</h1>
        </div>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 2fr", gap: 24 }}>
        {/* Left Column - Customer Info */}
        <div>
          {/* Profile Card */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-body" style={{ textAlign: "center", padding: 24 }}>
              <div style={{
                width: 80, height: 80, borderRadius: "50%", margin: "0 auto 16px",
                display: "flex", alignItems: "center", justifyContent: "center",
                backgroundColor: "#e5e7eb", fontSize: 28, fontWeight: 700, color: "#374151"
              }}>
                {customer.avatar_url ? (
                  <img src={customer.avatar_url} alt="" style={{ width: "100%", height: "100%", borderRadius: "50%", objectFit: "cover" }} />
                ) : initials}
              </div>
              <h3 style={{ marginBottom: 4 }}>{customer.full_name || "No Name"}</h3>
              <p style={{ color: "#6b7280", fontSize: 14, marginBottom: 8 }}>{customer.email}</p>
              {customer.phone && (
                <p style={{ color: "#6b7280", fontSize: 14 }}>{customer.phone}</p>
              )}
              <div style={{ display: "flex", gap: 8, justifyContent: "center", marginTop: 16 }}>
                <span style={{
                  padding: "4px 10px", borderRadius: 6, fontSize: 12, fontWeight: 600,
                  backgroundColor: customer.is_active ? "#f0fdf4" : "#fef2f2",
                  color: customer.is_active ? "#16a34a" : "#dc2626"
                }}>
                  {customer.is_active ? "Active" : "Inactive"}
                </span>
                {customer.email_verified && (
                  <span style={{
                    padding: "4px 10px", borderRadius: 6, fontSize: 12, fontWeight: 600,
                    backgroundColor: "#eff6ff", color: "#2563eb"
                  }}>
                    Verified
                  </span>
                )}
              </div>
            </div>
          </div>

          {/* Stats */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header">
              <h3 className="card-title">Customer Stats</h3>
            </div>
            <div className="card-body">
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                <span style={{ color: "#6b7280" }}>Total Orders:</span>
                <span style={{ fontWeight: 600 }}>{customer.order_count || 0}</span>
              </div>
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                <span style={{ color: "#6b7280" }}>Total Spent:</span>
                <span style={{ fontWeight: 600 }}>{formatCurrency(customer.total_spent || 0)}</span>
              </div>
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                <span style={{ color: "#6b7280" }}>Avg. Order:</span>
                <span style={{ fontWeight: 600 }}>
                  {customer.order_count > 0
                    ? formatCurrency((customer.total_spent || 0) / customer.order_count)
                    : formatCurrency(0)}
                </span>
              </div>
              <div style={{ display: "flex", justifyContent: "space-between" }}>
                <span style={{ color: "#6b7280" }}>Member Since:</span>
                <span style={{ fontWeight: 500, fontSize: 13 }}>
                  {customer.created_at
                    ? new Date(customer.created_at).toLocaleDateString("pt-BR", {
                        day: "2-digit", month: "short", year: "numeric"
                      })
                    : "-"}
                </span>
              </div>
            </div>
          </div>
        </div>

        {/* Right Column - Orders */}
        <div>
          <div className="admin-card">
            <div className="card-header">
              <h3 className="card-title">Order History ({orders.length})</h3>
            </div>
            <div className="card-body" style={{ padding: 0 }}>
              {orders.length === 0 ? (
                <div style={{ padding: 40, textAlign: "center", color: "#9ca3af" }}>
                  No orders found for this customer.
                </div>
              ) : (
                <table className="admin-table">
                  <thead>
                    <tr>
                      <th>Order</th>
                      <th>Date</th>
                      <th>Status</th>
                      <th>Payment</th>
                      <th style={{ textAlign: "right" }}>Total</th>
                    </tr>
                  </thead>
                  <tbody>
                    {orders.map(order => (
                      <tr
                        key={order.id}
                        style={{ cursor: "pointer" }}
                        onClick={() => router.push(`/admin/orders/${order.id}`)}
                      >
                        <td style={{ fontWeight: 500, fontFamily: "monospace", fontSize: 13 }}>
                          {order.order_number}
                        </td>
                        <td style={{ fontSize: 13, color: "#6b7280" }}>
                          {formatDate(order.created_at)}
                        </td>
                        <td>
                          <StatusBadge status={order.status} type="order" />
                        </td>
                        <td>
                          <StatusBadge status={order.payment_status} type="payment" />
                        </td>
                        <td style={{ textAlign: "right", fontWeight: 600 }}>
                          {formatCurrency(order.total)}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
